
#include "debugger/dsp.h"

#include "common/sys.h"


static const char *RegisterNames[] = {
	"ar0", "ar1", "ar2", "ar3",
	"ix0", "ix1", "ix2", "ix3",
	"wr0", "wr1", "wr2", "wr3",
	"st0", "st1", "st2", "st3",
	"ac0h", "ac1h", "config", "sr",
	"prodl", "prodm1", "prodh", "prodm2",
	"ax0l", "ax1l", "ax0h", "ax1h",
	"ac0l", "ac1l", "ac0m", "ac1m"
};


DSPDebugger::DSPDebugger(DSPInterpreter *cpu) {
	this->cpu = cpu;
}

Processor *DSPDebugger::getProcessor() {
	return cpu;
}

std::string DSPDebugger::name() {
	return "DSP";
}

std::string DSPDebugger::format() {
	return "%04X";
}

Ref<EvalContext> DSPDebugger::getContext() {
	Ref<EvalContext> context = new EvalContext();
	
	for (int i = 0; i < 32; i++) {
		if (DSPInterpreter::ST0 <= i && i <= DSPInterpreter::ST3) continue;
		
		uint16_t value = cpu->readreg(i);
		
		std::string reg = StringUtils::format("r%i", i);
		
		context->add(RegisterNames[i], value);
		context->add(reg, value);
	}
	context->add("pc", cpu->pc);
	return context;
}

uint32_t DSPDebugger::pc() {
	return cpu->pc;
}

bool DSPDebugger::translate(uint32_t *address) {
	return true;
}

Buffer DSPDebugger::read(uint32_t address, uint32_t length, bool code) {
	std::pair<void *, uint32_t> pair = getPtr(address, code);
	if (!pair.first) {
		return Buffer();
	}
	
	if (length > pair.second * 2) {
		length = pair.second * 2;
	}
	
	return Buffer(pair.first, length, Buffer::CreateCopy);
}

std::pair<void *, uint32_t> DSPDebugger::getPtr(uint32_t address, bool code) {
	if (code) {
		if (address < 0x2000) {
			return std::make_pair(cpu->iram + address, 0x2000 - address);
		}
		if (0x8000 <= address && address < 0x9000) {
			address -= 0x8000;
			return std::make_pair(cpu->irom + address, 0x1000 - address);
		}
	}
	else {
		if (address < 0x3000) {
			return std::make_pair(cpu->dram + address, 0x3000 - address);
		}
		if (address < 0x3800) {
			address -= 0x3000;
			return std::make_pair(cpu->drom + address, 0x800 - address);
		}
	}
	return std::make_pair(nullptr, 0);
}

void DSPDebugger::printState() {
	Sys::out->write(
		"ar0 = %04X ar1 = %04X ar2 = %04X ar3 = %04X\n"
		"ix0 = %04X ix1 = %04X ix2 = %04X ix3 = %04X\n"
		"wr0 = %04X wr1 = %04X wr2 = %04X wr3 = %04X\n\n"
		"ac0 = %02X%04X%04X ac1 = %02X%04X%04X\n"
		"ax0 =   %04X%04X ax1 =   %04X%04X\n\n"
		"config = %04X sr = %04X\n\n",
		cpu->ar[0], cpu->ar[1], cpu->ar[2], cpu->ar[3],
		cpu->ix[0], cpu->ix[1], cpu->ix[2], cpu->ix[3],
		cpu->wr[0], cpu->wr[1], cpu->wr[2], cpu->wr[3],
		cpu->ac[0].h, cpu->ac[0].m, cpu->ac[0].l,
		cpu->ac[1].h, cpu->ac[1].m, cpu->ac[1].l,
		cpu->ax[0].h, cpu->ax[0].l, cpu->ax[1].h, cpu->ax[1].l,
		cpu->config, cpu->status
	);
	
	for (int i = 0; i < 4; i++) {
		printStack(i);
	}
}

void DSPDebugger::printStateDetailed() {
	printState();
}

void DSPDebugger::printStack(int index) {
	Sys::out->write("st%i = {", index);
	for (int i = 0; i < cpu->st[index].size(); i++) {
		if (i != 0) {
			Sys::out->write(", ");
		}
		Sys::out->write("%04X", cpu->st[index].get(i));
	}
	Sys::out->write("}\n");
}

void DSPDebugger::printStackTrace() {
	Sys::out->write("pc = %04X\n", cpu->pc);
	
	Sys::out->write("st0 = {");
	for (int i = 0; i < cpu->st[0].size(); i++) {
		Sys::out->write("\n");
		if (i != 0) {
			Sys::out->write(",\n");
		}
		Sys::out->write("    %04X", cpu->st[0].get(i));
	}
	Sys::out->write("\n}\n");
}

void DSPDebugger::printMemoryMap() {
	Sys::out->write("DSP does not have virtual memory.");
}

void DSPDebugger::printThreads() {
	Sys::out->write("DSP does not have threads.");
}

void DSPDebugger::printThreadDetails(uint32_t id) {
	Sys::out->write("DSP does not have threads.");
}

#if STATS
void DSPDebugger::printStats() {
	uint64_t totalTransfers = 0;
	for (int i = 0; i < 4; i++) {
		totalTransfers += cpu->dma_transfers[i];
	}
	
	Sys::out->write("\n");
	Sys::out->write("DSP:\n");
	Sys::out->write("    Instructions executed: %i\n", cpu->instrs_executed);
	Sys::out->write("    \n");
	Sys::out->write("    DMA transfers: %i\n", totalTransfers);
	Sys::out->write("    CPU to IRAM: %i\n", cpu->dma_transfers[2]);
	Sys::out->write("    CPU to DRAM: %i\n", cpu->dma_transfers[0]);
	Sys::out->write("    IRAM to CPU: %i\n", cpu->dma_transfers[3]);
	Sys::out->write("    DRAM to CPU: %i\n", cpu->dma_transfers[1]);
}
#endif
