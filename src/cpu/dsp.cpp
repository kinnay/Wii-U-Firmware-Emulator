
#include "cpu/dsp.h"

#include "common/exceptions.h"
#include "common/fileutils.h"
#include "common/logger.h"
#include "common/buffer.h"
#include "common/endian.h"

#include "physicalmemory.h"

#include "config.h"

#include <cstring>


static int64_t sign8(uint8_t value) {
	if (value & 0x80) {
		return (int64_t)value - 0x100;
	}
	return value;
}


DSPStack::DSPStack(int capacity) {
	this->capacity = capacity;
}

void DSPStack::clear() {
	stack.clear();
}

void DSPStack::push(uint16_t value) {
	if (stack.size() == capacity) {
		runtime_error("DSP stack overflow");
	}
	stack.push_back(value);
}

uint16_t DSPStack::peek() {
	if (stack.empty()) {
		runtime_error("DSP stack underflow");
	}
	return stack[stack.size() - 1];
}

uint16_t DSPStack::pop() {
	uint16_t value = peek();
	stack.pop_back();
	return value;
}

uint16_t DSPStack::get(int index) {
	if (index >= stack.size()) {
		runtime_error("DSP stack invalid index");
	}
	return stack[stack.size() - index - 1];
}

bool DSPStack::empty() {
	return stack.empty();
}

int DSPStack::size() {
	return stack.size();
}


DSPInterpreter::DSPInterpreter(Emulator *emulator) :
	Processor(emulator, 4, false),
	st {8, 8, 4, 4}
{
	Buffer irom_data = FileUtils::load("files/dsp_irom.bin");
	if (irom_data.size() > sizeof(irom)) {
		runtime_error("DSP IROM is too large!");
	}
	
	Buffer drom_data = FileUtils::load("files/dsp_drom.bin");
	if (drom_data.size() > sizeof(drom)) {
		runtime_error("DSP DROM is too large!");
	}
	
	memcpy(irom, irom_data.get(), irom_data.size());
	memcpy(drom, drom_data.get(), drom_data.size());
	
	dma_logger.init("logs/dspdma.txt");
}

void DSPInterpreter::reset() {
	memset(iram, 0, sizeof(iram));
	memset(dram, 0, sizeof(dram));
	memset(ar, 0, sizeof(ix));
	memset(ix, 0, sizeof(ix));
	memset(wr, 0, sizeof(ix));
	
	for (int i = 0; i < 4; i++) {
		st[i].clear();
	}
	
	for (int i = 0; i < 2; i++) {
		ac[i].value = 0;
		ax[i].value = 0;
	}
	
	config = 0;
	status = 0;
	
	pc = 0x8000;
	
	irq = false;
	
	timer = 10000;
	
	mailbox_in_h = 0;
	mailbox_in_l = 0;
	mailbox_out_h = 0;
	mailbox_out_l = 0;
	
	dma_addr_main = 0;
	dma_addr_dsp = 0;
	dma_control = 0;
	
	ffd2 = 0;
	
	#if STATS
	instrs_executed = 0;
	for (int i = 0; i < 4; i++) {
		dma_transfers[i] = 0;
	}
	#endif
}

uint16_t DSPInterpreter::fetch() {
	return read_code(pc++);
}

uint16_t DSPInterpreter::read_code(uint16_t addr) {
	if (addr < 0x2000) {
		return Endian::swap16(iram[addr]);
	}
	
	if (0x8000 <= addr && addr < 0x9000) {
		return Endian::swap16(irom[addr - 0x8000]);
	}
	
	runtime_error("DSP invalid instruction address: 0x%X", addr);
	return 0;
}

uint16_t DSPInterpreter::read(uint16_t addr) {
	#if WATCHPOINTS
	checkWatchpoints(false, false, addr, 1);
	#endif
	
	if (addr < 0x3000) return Endian::swap16(dram[addr]);
	if (addr < 0x3800) return Endian::swap16(drom[addr - 0x3000]);
	
	if (addr == DSCR) return dma_control;
	if (addr == DSBL) return 0;
	
	if (addr == FFD2) return ffd2;
	
	if (addr == DMBH) return mailbox_out_h;
	if (addr == CMBH) return mailbox_in_h;
	if (addr == CMBL) {
		mailbox_in_h &= 0x7FFF;
		return mailbox_in_l;
	}
	
	Logger::warning("Unknown DSP read at 0x%X: 0x%X", pc, addr);
	return 0;
}

void DSPInterpreter::write(uint16_t addr, uint16_t value) {
	#if WATCHPOINTS
	checkWatchpoints(true, false, addr, 1);
	#endif
	
	if (addr < 0x3000) dram[addr] = Endian::swap16(value);
	
	else if (addr == DMA_ADDR_MAIN_H) {}
	else if (addr == DRC_AI_BUFF_L) {}
	else if (addr == DRC_AI_BUFF_M) {}
	else if (addr == DRC_AI_BUFF_H) {}
	else if (addr == TV_AI_BUFF_L) {}
	else if (addr == TV_AI_BUFF_M) {}
	else if (addr == TV_AI_BUFF_H) {}
	
	else if (addr == DSCR) dma_control = value;
	else if (addr == DSBL) {
		if (value & 1) {
			runtime_error("DSP DMA length is not aligned");
		}

		void *ptr = dma_ptr(dma_addr_dsp, value / 2, dma_control & 2);
		if (!ptr) {
			runtime_error("DSP DMA address out of range");
		}
		
		if (dma_control & 1) {
			physmem->write(dma_addr_main, ptr, value);
		}
		else {
			physmem->read(dma_addr_main, ptr, value);
		}
		
		#if DSPDMA
		std::string name1 = "CPU";
		std::string name2 = dma_control & 2 ? "IRAM" : "DRAM";
		
		std::string addr1 = StringUtils::format("%08X", dma_addr_main);
		std::string addr2 = StringUtils::format("%04X", dma_addr_dsp);
		
		if (dma_control & 1) {
			std::swap(name1, name2);
			std::swap(addr1, addr2);
		}
		
		std::string message = StringUtils::format(
			"%04X: %s -> %s, %s -> %s (%i bytes)\n",
			pc, name1, name2, addr1, addr2, value
		);
		dma_logger.write(message);
		#endif
		
		#if WATCHPOINTS
		if (!(dma_control & 2)) {
			checkWatchpoints(!(dma_control & 1), false, dma_addr_dsp, value / 2);
		}
		#endif
		
		#if STATS
		dma_transfers[dma_control & 3]++;
		#endif
	}
	else if (addr == DSPA) dma_addr_dsp = value;
	else if (addr == DSMAH) {
		value &= 0x7FFF;
		dma_addr_main &= 0xFFFF;
		dma_addr_main |= value << 16;
	}
	else if (addr == DSMAL) {
		dma_addr_main &= ~0xFFFF;
		dma_addr_main |= value;
	}
	
	else if (addr == FFD2) ffd2 = value;
	
	else if (addr == DIRQ) irq = value & 1;
	
	else if (addr == DMBH) mailbox_out_h = value;
	else if (addr == DMBL) {
		mailbox_out_l = value;
		mailbox_out_h |= 0x8000;
	}
	
	else {
		Logger::warning("Unknown DSP write at 0x%X: 0x%X (0x%04X)", pc, addr, value);
	}
}

void *DSPInterpreter::dma_ptr(uint16_t addr, uint16_t length, bool code) {
	if (code) {
		if (addr < 0x2000 && length <= 0x2000 - addr) {
			return iram + addr;
		}
	}
	else {
		if (addr < 0x3000 && length <= 0x3000 - addr) {
			return dram + addr;
		}
	}
	return nullptr;
}

uint16_t DSPInterpreter::readreg(int reg) {
	if (AR0 <= reg && reg <= AR3) return ar[reg - AR0];
	if (IX0 <= reg && reg <= IX3) return ix[reg - IX0];
	if (WR0 <= reg && reg <= WR3) return wr[reg - WR0];
	if (ST0 <= reg && reg <= ST3) return st[reg - ST0].pop();
	
	if (reg == AX0L) return ax[0].l;
	if (reg == AX1L) return ax[1].l;
	if (reg == AX0H) return ax[0].h;
	if (reg == AX1H) return ax[1].h;
	
	if (reg == AC0L) return ac[0].l;
	if (reg == AC1L) return ac[1].l;
	if (reg == AC0M) return ac[0].m;
	if (reg == AC1M) return ac[1].m;
	if (reg == AC0H) return ac[0].h;
	if (reg == AC1H) return ac[1].h;
	
	if (reg == CONFIG) return config;
	if (reg == SR) return status;
	
	return 0;
}

void DSPInterpreter::writereg(int reg, uint16_t value) {
	if (AR0 <= reg && reg <= AR3) ar[reg - AR0] = value;
	else if (IX0 <= reg && reg <= IX3) ix[reg - IX0] = value;
	else if (ST0 <= reg && reg <= ST3) st[reg - ST0].push(value);
	
	else if (reg == CONFIG) config = value;
	else if (reg == SR) status = value;
	
	else if (reg == AX0L || reg == AX1L) ax[reg - AX0L].l = value;
	else if (reg == AX0H || reg == AX1H) ax[reg - AX0H].h = value;
	
	else if (reg == AC0L || reg == AC1L) ac[reg - AC0L].l = value;
	else if (reg == AC0H || reg == AC1H) ac[reg - AC0H].h = value;
	else if (reg == AC0M || reg == AC1M) {
		ac[reg - AC0M].m = value;
		if (status.get(SXM)) {
			ac[reg - AC0M].h = value & 0x8000 ? 0xFF : 0;
			ac[reg - AC0M].l = 0;
		}
	}
	else if (reg == AC1L) ac[1].l = value;
	else if (reg == AC0M) ac[0].m = value;
	else if (reg == AC1M) ac[1].m = value;
	else if (reg == AC0H) ac[0].h = value;
	else if (reg == AC1H) ac[1].h = value;
	else {
		runtime_error("Unsupported DSP register: %i", reg);
	}
}

// When the dsp_os_switch task receives the 'stop' message
// it waits until bit 1 << 2 is set in @dspState. This bit
// is only set by the exception handler at address 0xC, but
// this exception isn't documented or implemented by dolphin-emu.
// 
// The PPC doesn't seem to do anything that might trigger
// the exception. For now I'm assuming that the exception
// is triggered periodically by the DSP itself. This at
// least prevents the DSP from getting stuck while waiting
// for the bit in @dspState.
void DSPInterpreter::update_timer() {
	if (!(status.get(1 << 10))) { // No idea if this is correct
		if (timer-- == 0) {
			trigger_exception(6);
			timer = 10000; // This is arbitrary
		}
	}
}

void DSPInterpreter::trigger_exception(int type) {
	writereg(ST0, pc);
	writereg(ST1, status);
	status.set(1 << 10, true);
	pc = type * 2;
}

void DSPInterpreter::step() {
	update_timer();
	
	uint16_t prev = this->pc;
	
	uint16_t instr = fetch();
	if (instr == 0) nop(instr);
	else if ((instr & 0xFFFC) == 0x0008) iar(instr);
	else if ((instr & 0xFFE0) == 0x0040) loop(instr);
	else if ((instr & 0xFFE0) == 0x0060) bloop(instr);
	else if ((instr & 0xFFE0) == 0x0080) lri(instr);
	else if ((instr & 0xFFE0) == 0x00C0) lr(instr);
	else if ((instr & 0xFFE0) == 0x00E0) sr(instr);
	else if ((instr & 0xFEFC) == 0x0210) ilrr(instr);
	else if ((instr & 0xFEFC) == 0x0218) ilrri(instr);
	else if ((instr & 0xFEFF) == 0x0240) andi(instr);
	else if ((instr & 0xFEFF) == 0x0260) ori(instr);
	else if ((instr & 0xFFF0) == 0x0290) jcc(instr);
	else if ((instr & 0xFEFF) == 0x02A0) andf(instr);
	else if ((instr & 0xFFFF) == 0x02BF) call(instr);
	else if ((instr & 0xFEFF) == 0x02C0) andcf(instr);
	else if ((instr & 0xFFFF) == 0x02DF) ret(instr);
	else if ((instr & 0xFFFF) == 0x02FF) rti(instr);
	else if ((instr & 0xFE00) == 0x0400) addis(instr);
	else if ((instr & 0xF800) == 0x0800) lris(instr);
	else if ((instr & 0xFF00) == 0x1100) bloopi(instr);
	else if ((instr & 0xFFF8) == 0x1200) sbset(instr);
	else if ((instr & 0xFFF8) == 0x1300) sbclr(instr);
	else if ((instr & 0xFEC0) == 0x1400) lsl(instr);
	else if ((instr & 0xFEC0) == 0x1440) lsr(instr);
	else if ((instr & 0xFF00) == 0x1600) si(instr);
	else if ((instr & 0xFF1F) == 0x170F) jmpr(instr);
	else if ((instr & 0xFF1F) == 0x171F) callr(instr);
	else if ((instr & 0xFF80) == 0x1800) lrr(instr);
	else if ((instr & 0xFF80) == 0x1900) lrri(instr);
	else if ((instr & 0xFF80) == 0x1980) lrrn(instr);
	else if ((instr & 0xFF80) == 0x1A00) srr(instr);
	else if ((instr & 0xFF80) == 0x1B00) srri(instr);
	else if ((instr & 0xFF80) == 0x1B80) srrn(instr);
	else if ((instr & 0xFC00) == 0x1C00) mrr(instr);
	else if ((instr & 0xF800) == 0x2000) lrs(instr);
	else if ((instr & 0xF800) == 0x2800) srs(instr);
	else if ((instr & 0xFC80) == 0x3000) xorr(instr);
	else if ((instr & 0xFC80) == 0x3080) xorc(instr);
	else if ((instr & 0xFC80) == 0x3400) andr(instr);
	else if ((instr & 0xFC80) == 0x3800) orr(instr);
	else if ((instr & 0xFE80) == 0x3E00) orc(instr);
	else if ((instr & 0xF800) == 0x4000) addr(instr);
	else if ((instr & 0xFC00) == 0x4800) addax(instr);
	else if ((instr & 0xFE00) == 0x4C00) add(instr);
	else if ((instr & 0xF800) == 0x5000) subr(instr);
	else if ((instr & 0xFE00) == 0x5C00) sub(instr);
	else if ((instr & 0xFE00) == 0x7400) incm(instr);
	else if ((instr & 0xFE00) == 0x7600) inc(instr);
	else if ((instr & 0xF700) == 0x8100) clr(instr);
	else if ((instr & 0xFF00) == 0x8200) cmp(instr);
	else if ((instr & 0xFE00) == 0x8A00) setam(instr);
	else if ((instr & 0xFE00) == 0x8C00) setsu(instr);
	else if ((instr & 0xFE00) == 0x8E00) sxm(instr);
	else if ((instr & 0xF700) == 0xB100) tst(instr);
	else {
		runtime_error("Unknown DSP instruction at 0x%X: 0x%04X", pc - 1, instr);
	}
	
	if (!st[2].empty() && st[2].peek() == prev) {
		uint16_t counter = st[3].pop() - 1;
		if (counter == 0) {
			st[0].pop();
			st[2].pop();
		}
		else {
			st[3].push(counter);
			pc = st[0].peek();
		}
	}
	
	#if STATS
	instrs_executed++;
	#endif
	
	#if BREAKPOINTS
	checkBreakpoints(pc);
	#endif
}

void DSPInterpreter::skip() {
	uint16_t instr = fetch();
	if ((instr & 0xFFE0) == 0x0060) pc++; // bloop
	else if ((instr & 0xFFE0) == 0x0080) pc++; // lri
	else if ((instr & 0xFFE0) == 0x00C0) pc++; // lr
	else if ((instr & 0xFFE0) == 0x00E0) pc++; // sr
	else if ((instr & 0xFEFF) == 0x0240) pc++; // andi
	else if ((instr & 0xFEFF) == 0x0260) pc++; // ori
	else if ((instr & 0xFFF0) == 0x0290) pc++; // jcc
	else if ((instr & 0xFEFF) == 0x02A0) pc++; // andf
	else if ((instr & 0xFFFF) == 0x02BF) pc++; // call
	else if ((instr & 0xFEFF) == 0x02C0) pc++; // andcf
	else if ((instr & 0xFF00) == 0x1100) pc++; // bloopi
	else if ((instr & 0xFF00) == 0x1600) pc++; // si
}

bool DSPInterpreter::checkcond(int cond) {
	if (cond == 0) return status.get(O) == status.get(S);
	if (cond == 1) return status.get(O) != status.get(S);
	if (cond == 2) return status.get(O) == status.get(S) && !status.get(Z);
	if (cond == 4) return !status.get(Z);
	if (cond == 5) return status.get(Z);
	if (cond == 12) return !status.get(LZ);
	if (cond == 13) return status.get(LZ);
	if (cond == 15) return true;
	
	runtime_error("Unsupported DSP condition: %i", cond);
	return false;
}

void DSPInterpreter::updateflags(uint64_t value) {
	status.set(Z, value == 0);
	status.set(S, value >> 39);
}

uint64_t DSPInterpreter::add_with_flags(uint64_t v1, uint64_t v2) {
	v1 &= 0xFFFFFFFFFF;
	v2 &= 0xFFFFFFFFFF;
	
	uint64_t result = v1 + v2;
	bool s1 = v1 >> 39;
	bool s2 = v2 >> 39;
	bool s3 = (result >> 39) & 1;
	status.set(C, result >> 40);
	status.set(O, s1 == s2 && s1 != s3);
	
	result &= 0xFFFFFFFFFF;
	
	updateflags(result);
	return result;
}

void DSPInterpreter::doext(uint16_t instr) {
	uint8_t ext = instr & 0xFF;
	if (ext) {
		runtime_error("Unknown extended opcode at 0x%X: 0x%02X", pc, ext);
	}
}

void DSPInterpreter::add(uint16_t instr) {
	int d = (instr >> 8) & 1;
	ac[d].value = add_with_flags(ac[d].value, ac[1 - d].value);
	doext(instr & 0xFF);
}

void DSPInterpreter::addax(uint16_t instr) {
	int d = (instr >> 8) & 1;
	int s = (instr >> 9) & 1;
	ac[d].value = add_with_flags(ac[d].value, ax[s].value);
	doext(instr & 0xFF);
}

void DSPInterpreter::addis(uint16_t instr) {
	int d = (instr >> 8) & 1;
	ac[d].value = add_with_flags(ac[d].value, sign8(instr & 0xFF) << 16);
}

void DSPInterpreter::addr(uint16_t instr) {
	int d = (instr >> 8) & 1;
	int s = (instr >> 9) & 3;
	ac[d].value = add_with_flags(ac[d].value, readreg(24 + s) << 16);
	doext(instr & 0xFF);
}

void DSPInterpreter::andcf(uint16_t instr) {
	uint16_t mask = fetch();
	uint16_t result = ac[(instr >> 8) & 1].m & mask;
	status.set(LZ, result == mask);
}

void DSPInterpreter::andf(uint16_t instr) {
	uint16_t mask = fetch();
	uint16_t result = ac[(instr >> 8) & 1].m & mask;
	status.set(LZ, result == 0);
}

void DSPInterpreter::andi(uint16_t instr) {
	int r = (instr >> 8) & 1;
	ac[r].m &= fetch();
	updateflags(ac[r].value);
}

void DSPInterpreter::andr(uint16_t instr) {
	int d = (instr >> 8) & 1;
	int s = (instr >> 9) & 1;
	ac[d].m &= ax[s].h;
	updateflags(ac[d].value);
	doext(instr & 0x7F);
}

void DSPInterpreter::bloop(uint16_t instr) {
	uint16_t count = readreg(instr & 0x1F);
	uint16_t end = fetch();
	if (count) {
		writereg(ST0, pc);
		writereg(ST2, end);
		writereg(ST3, count);
	}
	else {
		pc = end;
		skip();
	}
}

void DSPInterpreter::bloopi(uint16_t instr) {
	uint16_t count = instr & 0xFF;
	uint16_t end = fetch();
	if (count) {
		writereg(ST0, pc);
		writereg(ST2, end);
		writereg(ST3, count);
	}
	else {
		pc = end;
		skip();
	}
}

void DSPInterpreter::call(uint16_t instr) {
	writereg(ST0, pc + 1);
	pc = fetch();
}

void DSPInterpreter::callr(uint16_t instr) {
	writereg(ST0, pc);
	pc = readreg((instr >> 5) & 7);
}

void DSPInterpreter::clr(uint16_t instr) {
	ac[(instr >> 11) & 1].value = 0;
	updateflags(0);
	doext(instr & 0xFF);
}

void DSPInterpreter::cmp(uint16_t instr) {
	add_with_flags(ac[0].value, -ac[1].value);
	doext(instr & 0xFF);
}

void DSPInterpreter::iar(uint16_t instr) {
	ar[instr & 3]++;
}

void DSPInterpreter::ilrr(uint16_t instr) {
	ac[(instr >> 8) & 1].m = read_code(ar[instr & 3]);
}

void DSPInterpreter::ilrri(uint16_t instr) {
	ac[(instr >> 8) & 1].m = read_code(ar[instr & 3]++);
}

void DSPInterpreter::inc(uint16_t instr) {
	int d = (instr >> 8) & 1;
	ac[d].value = add_with_flags(ac[d].value, 1);
	doext(instr & 0xFF);
}

void DSPInterpreter::incm(uint16_t instr) {
	int d = (instr >> 8) & 1;
	ac[d].value = add_with_flags(ac[d].value, 1 << 16);
	doext(instr & 0xFF);
}

void DSPInterpreter::jcc(uint16_t instr) {
	uint16_t target = fetch();
	if (checkcond(instr & 0xF)) {
		pc = target;
	}
}

void DSPInterpreter::jmpr(uint16_t instr) {
	pc = readreg((instr >> 5) & 7);
}

void DSPInterpreter::loop(uint16_t instr) {
	uint16_t count = readreg(instr & 0x1F);
	if (count) {
		writereg(ST0, pc);
		writereg(ST2, pc);
		writereg(ST3, count);
	}
	else {
		skip();
	}
}

void DSPInterpreter::lr(uint16_t instr) {
	writereg(instr & 0x1F, read(fetch()));
}

void DSPInterpreter::lri(uint16_t instr) {
	writereg(instr & 0x1F, fetch());
}

void DSPInterpreter::lris(uint16_t instr) {
	writereg(24 + ((instr >> 8) & 7), sign8(instr & 0xFF));
}

void DSPInterpreter::lrr(uint16_t instr) {
	writereg(instr & 0x1F, read(ar[(instr >> 5) & 3]));
}

void DSPInterpreter::lrri(uint16_t instr) {
	writereg(instr & 0x1F, read(ar[(instr >> 5) & 3]++));
}

void DSPInterpreter::lrrn(uint16_t instr) {
	int s = (instr >> 5) & 3;
	writereg(instr & 0x1F, read(ar[s]));
	ar[s] += ix[s];
}

void DSPInterpreter::lrs(uint16_t instr) {
	writereg(24 + ((instr >> 8) & 7), read(sign8(instr & 0xFF)));
}

void DSPInterpreter::lsl(uint16_t instr) {
	int r = (instr >> 8) & 1;
	ac[r].value <<= instr & 0x3F;
	ac[r].value &= 0xFFFFFFFFFF;
	updateflags(ac[r].value);
}

void DSPInterpreter::lsr(uint16_t instr) {
	int r = (instr >> 8) & 1;
	ac[r].value >>= 64 - (instr & 0x3F);
	updateflags(ac[r].value);
}

void DSPInterpreter::mrr(uint16_t instr) {
	writereg((instr >> 5) & 0x1F, readreg(instr & 0x1F));
}

void DSPInterpreter::nop(uint16_t instr) {}

void DSPInterpreter::orc(uint16_t instr) {
	int d = (instr >> 8) & 1;
	ac[d].m |= ac[1 - d].m;
	updateflags(ac[d].value);
	doext(instr & 0x7F);
}

void DSPInterpreter::ori(uint16_t instr) {
	int d = (instr >> 8) & 1;
	ac[d].m |= fetch();
	updateflags(ac[d].value);
}

void DSPInterpreter::orr(uint16_t instr) {
	int d = (instr >> 8) & 1;
	int s = (instr >> 9) & 1;
	ac[d].m |= ax[s].h;
	updateflags(ac[d].value);
	doext(instr & 0x7F);
}

void DSPInterpreter::ret(uint16_t instr) {
	pc = readreg(ST0);
}

void DSPInterpreter::rti(uint16_t instr) {
	status = readreg(ST1);
	pc = readreg(ST0);
}

void DSPInterpreter::sbclr(uint16_t instr) {
	status.set(1 << ((instr & 7) + 6), false);
}

void DSPInterpreter::sbset(uint16_t instr) {
	status.set(1 << ((instr & 7) + 6), true);
}

void DSPInterpreter::setam(uint16_t instr) {
	status.set(AM, (instr >> 8) & 1);
	doext(instr & 0xFF);
}

void DSPInterpreter::setsu(uint16_t instr) {
	status.set(SU, (instr >> 8) & 1);
	doext(instr & 0xFF);
}

void DSPInterpreter::si(uint16_t instr) {
	write(sign8(instr & 0xFF), fetch());
}

void DSPInterpreter::sr(uint16_t instr) {
	write(fetch(), readreg(instr & 0x1F));
}

void DSPInterpreter::srr(uint16_t instr) {
	int d = (instr >> 5) & 3;
	write(ar[d], readreg(instr & 0x1F));
}

void DSPInterpreter::srri(uint16_t instr) {
	int d = (instr >> 5) & 3;
	write(ar[d]++, readreg(instr & 0x1F));
}

void DSPInterpreter::srrn(uint16_t instr) {
	int d = (instr >> 5) & 3;
	write(ar[d], readreg(instr & 0x1F));
	ar[d] += ix[d];
}

void DSPInterpreter::srs(uint16_t instr) {
	write(sign8(instr & 0xFF), readreg(24 + ((instr >> 8) & 7)));
}

void DSPInterpreter::sub(uint16_t instr) {
	int d = (instr >> 8) & 1;
	ac[d].value = add_with_flags(ac[d].value, -ac[1 - d].value);
	doext(instr & 0xFF);
}

void DSPInterpreter::subr(uint16_t instr) {
	int d = (instr >> 8) & 1;
	int s = (instr >> 9) & 3;
	ac[d].value = add_with_flags(ac[d].value, -(readreg(24 + s) << 16));
	doext(instr & 0xFF);
}

void DSPInterpreter::sxm(uint16_t instr) {
	status.set(SXM, (instr >> 8) & 1);
	doext(instr & 0xFF);
}

void DSPInterpreter::tst(uint16_t instr) {
	updateflags(ac[(instr >> 11) & 1].value);
	doext(instr & 0xFF);
}

void DSPInterpreter::xorc(uint16_t instr) {
	int d = (instr >> 8) & 1;
	ac[d].m ^= ac[1 - d].m;
	updateflags(ac[d].value);
	doext(instr & 0x7F);
}

void DSPInterpreter::xorr(uint16_t instr) {
	int d = (instr >> 8) & 1;
	int s = (instr >> 9) & 1;
	ac[d].m ^= ax[s].h;
	updateflags(ac[d].value);
	doext(instr & 0x7F);
}
