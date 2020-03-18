
#include "ppcjit.h"
#include "ppcinstruction.h"
#include "ppcprocessor.h"
#include "x86codegenerator.h"

#include "common/exceptions.h"

#include <sys/mman.h>

#include <cstring>


static const int PC = offsetof(PPCProcessor, core.pc);
static const int CR = offsetof(PPCProcessor, core.cr);
static_assert(CR < 0x80);

#define REG(i) (offsetof(PPCProcessor, core.regs) + (i) * 4)
#define SPR(r) (offsetof(PPCProcessor, core.sprs[PPCCore::r]))

#define FPR_PS0(i) (offsetof(PPCProcessor, core.fprs) + (i) * 8)
#define FPR_PS1(i) (offsetof(PPCProcessor, core.fprs) + (i) * 8 + 4)
#define FPR_IW0(i) (offsetof(PPCProcessor, core.fprs) + (i) * 8)
#define FPR_IW1(i) (offsetof(PPCProcessor, core.fprs) + (i) * 8 + 4)
#define FPR_DBL(i) (offsetof(PPCProcessor, core.fprs) + (i) * 8)


static uint32_t genmask(int start, int end) {
	if (start <= end) {
		return (0xFFFFFFFF >> (31 - (end - start))) << (31 - end);
	}
	return (0xFFFFFFFF >> start) | (0xFFFFFFFF << (31 - end));
}


void executeInstr(PPCProcessor *cpu, PPCInstruction instr) {
	#if STATS
	cpu->jit.instrsExecuted--;
	#endif
	
	instr.execute(cpu);
}

void throwInstr(PPCProcessor *cpu, PPCInstruction instr) {
	runtime_error("PPC instruction: 0x%08X (at 0x%08X)", instr.value, cpu->core.pc);
}

#if METRICS
void updateMetrics(PPCProcessor *cpu, PPCInstruction instr) {
	cpu->metrics.update(instr);
}
#endif

template <class T>
static bool loadMemory(PPCProcessor *cpu, uint32_t addr, uint32_t *ptr) {
	T value;
	if (!cpu->read<T>(addr, &value)) {
		return false;
	}
	*ptr = value;
	return true;
}

template <class T>
static bool storeMemory(PPCProcessor *cpu, uint32_t addr, uint32_t value) {
	return cpu->write<T>(addr, value);
}

bool storeLong(PPCProcessor *cpu, uint32_t addr, uint64_t value) {
	return cpu->write<uint64_t>(addr, value);
}


PPCCodeGenerator::PPCCodeGenerator() : generator(0x8000) {
	generator.seek(0x400 * 5);
	index = 0;
}

char *PPCCodeGenerator::get() {
	generator.seek(0);
	for (int i = 0; i < index; i++) {
		generator.jumpRel(offsets[i]);
	}
	return generator.get();
}

size_t PPCCodeGenerator::size() {
	return generator.size();
}

void PPCCodeGenerator::generate(PPCInstruction instr) {
	offsets[index] = generator.tell() - (index + 1) * 5;
	generateInstr(instr);
	index++;
}

void PPCCodeGenerator::generateInstr(PPCInstruction instr) {
	#if METRICS
	generateMetricsUpdate(instr);
	#endif
	
	int type = instr.opcode();
	if (type == 4) {
		int type = instr.opcode2();
		if (type == 18) generateUnimplemented(instr);
		else if (type == 20) generateUnimplemented(instr);
		else if (type == 21) generateUnimplemented(instr);
		else if (type == 40) generateUnimplemented(instr);
		else if (type == 72) generateUnimplemented(instr);
		else if (type == 136) generateUnimplemented(instr);
		else if (type == 264) generateUnimplemented(instr);
		else if (type == 528) generateUnimplemented(instr);
		else if (type == 560) generateUnimplemented(instr);
		else if (type == 592) generateUnimplemented(instr);
		else if (type == 624) generateUnimplemented(instr);
		else if (type == 1014) generator.ret(); // dcbz_l
		else {
			int type = instr.opcode3();
			if (type == 10) generateUnimplemented(instr);
			else if (type == 11) generateUnimplemented(instr);
			else if (type == 12) generateUnimplemented(instr);
			else if (type == 13) generateUnimplemented(instr);
			else if (type == 25) generateUnimplemented(instr);
			else if (type == 28) generateUnimplemented(instr);
			else if (type == 29) generateUnimplemented(instr);
			else if (type == 30) generateUnimplemented(instr);
			else if (type == 31) generateUnimplemented(instr);
			else {
				generateError(instr);
			}
		}
	}
	else if (type == 7) generateUnimplemented(instr);
	else if (type == 8) generateUnimplemented(instr);
	else if (type == 10) generateUnimplemented(instr);
	else if (type == 11) generateUnimplemented(instr);
	else if (type == 12) generateUnimplemented(instr);
	else if (type == 13) generateUnimplemented(instr);
	else if (type == 14) generateAddi(instr);
	else if (type == 15) generateAddis(instr);
	else if (type == 16) generateBc(instr);
	else if (type == 17) generateUnimplemented(instr);
	else if (type == 18) generateB(instr);
	else if (type == 19) {
		int type = instr.opcode2();
		if (type == 16) generateUnimplemented(instr);
		else if (type == 50) generateUnimplemented(instr);
		else if (type == 150) generator.ret(); // isync
		else if (type == 193) generateUnimplemented(instr);
		else if (type == 528) generateUnimplemented(instr);
		else {
			generateError(instr);
		}
	}
	else if (type == 20) generateRlwimi(instr);
	else if (type == 21) generateRlwinm(instr);
	else if (type == 24) generateOri(instr);
	else if (type == 25) generateOris(instr);
	else if (type == 26) generateXori(instr);
	else if (type == 27) generateXoris(instr);
	else if (type == 28) generateAndi(instr);
	else if (type == 29) generateAndis(instr);
	else if (type == 31) {
		int type = instr.opcode2();
		if (type == 0) generateUnimplemented(instr);
		else if (type == 8) generateUnimplemented(instr);
		else if (type == 10) generateUnimplemented(instr);
		else if (type == 11) generateUnimplemented(instr);
		else if (type == 19) generateUnimplemented(instr);
		else if (type == 20) generateUnimplemented(instr);
		else if (type == 23) generateUnimplemented(instr);
		else if (type == 24) generateUnimplemented(instr);
		else if (type == 26) generateUnimplemented(instr);
		else if (type == 28) generateUnimplemented(instr);
		else if (type == 32) generateUnimplemented(instr);
		else if (type == 40) generateUnimplemented(instr);
		else if (type == 54) generator.ret(); // dcbst
		else if (type == 55) generateUnimplemented(instr);
		else if (type == 60) generateUnimplemented(instr);
		else if (type == 75) generateUnimplemented(instr);
		else if (type == 83) generateUnimplemented(instr);
		else if (type == 86) generator.ret(); // dcbf
		else if (type == 87) generateUnimplemented(instr);
		else if (type == 104) generateUnimplemented(instr);
		else if (type == 119) generateUnimplemented(instr);
		else if (type == 124) generateUnimplemented(instr);
		else if (type == 136) generateUnimplemented(instr);
		else if (type == 138) generateUnimplemented(instr);
		else if (type == 144) generateUnimplemented(instr);
		else if (type == 146) generateUnimplemented(instr);
		else if (type == 150) generateUnimplemented(instr);
		else if (type == 151) generateUnimplemented(instr);
		else if (type == 183) generateUnimplemented(instr);
		else if (type == 200) generateUnimplemented(instr);
		else if (type == 202) generateUnimplemented(instr);
		else if (type == 210) generateUnimplemented(instr);
		else if (type == 215) generateUnimplemented(instr);
		else if (type == 235) generateUnimplemented(instr);
		else if (type == 247) generateUnimplemented(instr);
		else if (type == 266) generateAdd(instr);
		else if (type == 279) generateUnimplemented(instr);
		else if (type == 306) generateUnimplemented(instr);
		else if (type == 311) generateUnimplemented(instr);
		else if (type == 316) generateUnimplemented(instr);
		else if (type == 339) generateUnimplemented(instr);
		else if (type == 343) generateUnimplemented(instr);
		else if (type == 371) generateUnimplemented(instr);
		else if (type == 375) generateUnimplemented(instr);
		else if (type == 407) generateUnimplemented(instr);
		else if (type == 439) generateUnimplemented(instr);
		else if (type == 444) generateUnimplemented(instr);
		else if (type == 459) generateUnimplemented(instr);
		else if (type == 467) generateUnimplemented(instr);
		else if (type == 470) generator.ret(); // dcbi
		else if (type == 491) generateUnimplemented(instr);
		else if (type == 534) generateLwbrx(instr);
		else if (type == 535) generateLfsx(instr);
		else if (type == 536) generateUnimplemented(instr);
		else if (type == 567) generateUnimplemented(instr);
		else if (type == 595) generateUnimplemented(instr);
		else if (type == 598) generator.ret(); // sync
		else if (type == 599) generateLfdx(instr);
		else if (type == 631) generateStwbrx(instr);
		else if (type == 662) generateStwbrx(instr);
		else if (type == 663) generateStfsx(instr);
		else if (type == 695) generateStfsx(instr);
		else if (type == 727) generateStfdx(instr);
		else if (type == 759) generateUnimplemented(instr);
		else if (type == 792) generateUnimplemented(instr);
		else if (type == 824) generateUnimplemented(instr);
		else if (type == 854) generator.ret(); // eieio
		else if (type == 922) generateUnimplemented(instr);
		else if (type == 954) generateUnimplemented(instr);
		else if (type == 982) generateUnimplemented(instr);
		else if (type == 983) generateStfiwx(instr);
		else if (type == 1014) generateUnimplemented(instr);
		else {
			generateError(instr);
		}
	}
	else if (type == 32) generateLoad<uint32_t>(instr);
	else if (type == 33) generateLoadu<uint32_t>(instr);
	else if (type == 34) generateLoad<uint8_t>(instr);
	else if (type == 35) generateLoadu<uint8_t>(instr);
	else if (type == 36) generateUnimplemented(instr);
	else if (type == 37) generateUnimplemented(instr);
	else if (type == 38) generateUnimplemented(instr);
	else if (type == 39) generateUnimplemented(instr);
	else if (type == 40) generateLoad<uint16_t>(instr);
	else if (type == 41) generateLoadu<uint16_t>(instr);
	else if (type == 42) generateLoad<int16_t>(instr);
	else if (type == 43) generateLoadu<int16_t>(instr);
	else if (type == 44) generateUnimplemented(instr);
	else if (type == 45) generateUnimplemented(instr);
	else if (type == 46) generateUnimplemented(instr);
	else if (type == 47) generateUnimplemented(instr);
	else if (type == 48) generateUnimplemented(instr);
	else if (type == 49) generateUnimplemented(instr);
	else if (type == 50) generateUnimplemented(instr);
	else if (type == 51) generateUnimplemented(instr);
	else if (type == 52) generateUnimplemented(instr);
	else if (type == 53) generateUnimplemented(instr);
	else if (type == 54) generateUnimplemented(instr);
	else if (type == 55) generateUnimplemented(instr);
	else if (type == 56) generateUnimplemented(instr);
	else if (type == 59) {
		int type = instr.opcode3();
		if (type == 18) generateUnimplemented(instr);
		else if (type == 20) generateUnimplemented(instr);
		else if (type == 21) generateUnimplemented(instr);
		else if (type == 25) generateUnimplemented(instr);
		else if (type == 28) generateUnimplemented(instr);
		else if (type == 29) generateUnimplemented(instr);
		else if (type == 30) generateUnimplemented(instr);
		else if (type == 31) generateUnimplemented(instr);
		else {
			generateError(instr);
		}
	}
	else if (type == 60) generateUnimplemented(instr);
	else if (type == 63) {
		int type = instr.opcode2();
		if (type == 0) generateUnimplemented(instr);
		else if (type == 12) generateUnimplemented(instr);
		else if (type == 15) generateUnimplemented(instr);
		else if (type == 18) generateUnimplemented(instr);
		else if (type == 20) generateUnimplemented(instr);
		else if (type == 21) generateUnimplemented(instr);
		else if (type == 26) generateUnimplemented(instr);
		else if (type == 40) generateUnimplemented(instr);
		else if (type == 72) generateUnimplemented(instr);
		else if (type == 264) generateUnimplemented(instr);
		else if (type == 583) generateUnimplemented(instr);
		else if (type == 711) generateUnimplemented(instr);
		else {
			int type = instr.opcode3();
			if (type == 23) generateUnimplemented(instr);
			else if (type == 25) generateUnimplemented(instr);
			else if (type == 28) generateUnimplemented(instr);
			else if (type == 29) generateUnimplemented(instr);
			else if (type == 30) generateUnimplemented(instr);
			else if (type == 31) generateUnimplemented(instr);
			else {
				generateError(instr);
			}
		}
	}
	else {
		generateError(instr);
	}
}

#if METRICS
void PPCCodeGenerator::generateMetricsUpdate(PPCInstruction instr) {
	generator.pushReg64(RDI);
	generator.movImm32(RSI, instr.value);
	generator.callAbs(RAX, (uint64_t)updateMetrics);
	generator.popReg64(RDI);
}
#endif

void PPCCodeGenerator::generateError(PPCInstruction instr) {
	generator.movImm32(RSI, instr.value);
	generator.jumpAbs(RAX, (uint64_t)throwInstr);
}

void PPCCodeGenerator::generateUnimplemented(PPCInstruction instr) {
	generator.movImm32(RSI, instr.value);
	generator.jumpAbs(RAX, (uint64_t)executeInstr);
}

template <class T>
void PPCCodeGenerator::generateLoad(PPCInstruction instr) {
	if (instr.rA()) {
		generator.loadMem32(RSI, RDI, REG(instr.rA()));
		generator.addRegImm32(RSI, instr.d());
	}
	else {
		generator.movImm32(RSI, instr.d());
	}
	generator.lea64(RDX, RDI, REG(instr.rD()));
	generator.jumpAbs(RAX, (uint64_t)loadMemory<T>);
}

template <class T>
void PPCCodeGenerator::generateLoadu(PPCInstruction instr) {
	generator.pushReg64(RDI);
	generator.loadMem32(RSI, RDI, REG(instr.rA()));
	generator.addRegImm32(RSI, instr.d());
	generator.lea64(RDX, RDI, REG(instr.rD()));
	generator.callAbs(RAX, (uint64_t)loadMemory<T>);
	generator.popReg64(RDI);
	generator.testReg32(RAX, RAX);
	generator.jumpIfNotZero(1);
	generator.ret();
	generator.addMemImm32(RDI, REG(instr.rA()), instr.d());
	generator.ret();
}

void PPCCodeGenerator::generateConditionCheck(PPCInstruction instr) {
	int bo = instr.bo();
	if (!(bo & 4)) {
		generator.decMem32(RDI, SPR(CTR));
		if (bo & 2) {
			generator.jumpIfZero(1);
		}
		else {
			generator.jumpIfNotZero(1);
		}
		generator.ret();
	}
	if (!(bo & 0x10)) {
		generator.bitTestMem32(RDI, CR, 31 - instr.bi());
		if (bo & 8) {
			generator.jumpIfCarry(1);
		}
		else {
			generator.jumpIfNotCarry(1);
		}
		generator.ret();
	}
}

void PPCCodeGenerator::generateB(PPCInstruction instr) {
	if (instr.lk()) {
		generator.loadMem32(RAX, RDI, PC);
		generator.storeMem32(RDI, SPR(LR), RAX);
	}
	if (instr.aa()) {
		generator.storeMemImm32(RDI, PC, instr.li());
	}
	else {
		generator.addMemImm32(RDI, PC, instr.li() - 4);
	}
	generator.ret();
}

void PPCCodeGenerator::generateBc(PPCInstruction instr) {
	if (instr.lk()) {
		generator.loadMem32(RAX, RDI, PC);
		generator.storeMem32(RDI, SPR(LR), RAX);
	}
	
	generateConditionCheck(instr);
	if (instr.aa()) {
		generator.storeMemImm32(RDI, PC, instr.bd());
	}
	else {
		generator.addMemImm32(RDI, PC, instr.bd() - 4);
	}
	generator.ret();
}

void PPCCodeGenerator::generateAddi(PPCInstruction instr) {
	if (instr.rA()) {
		generator.loadMem32(RAX, RDI, REG(instr.rA()));
		generator.addRegImm32(RAX, instr.simm());
		generator.storeMem32(RDI, REG(instr.rD()), RAX);
	}
	else {
		generator.storeMemImm32(RDI, REG(instr.rD()), instr.simm());
	}
	generator.ret();
}

void PPCCodeGenerator::generateAddis(PPCInstruction instr) {
	if (instr.rA()) {
		generator.loadMem32(RAX, RDI, REG(instr.rA()));
		generator.addRegImm32(RAX, instr.simm() << 16);
		generator.storeMem32(RDI, REG(instr.rD()), RAX);
	}
	else {
		generator.storeMemImm32(RDI, REG(instr.rD()), instr.simm() << 16);
	}
	generator.ret();
}

void PPCCodeGenerator::generateRlwimi(PPCInstruction instr) {
	uint32_t mask = genmask(instr.mb(), instr.me());
	
	generator.loadMem32(RAX, RDI, REG(instr.rS()));
	generator.rolImm32(RAX, instr.sh());
	generator.andImm32(RAX, mask);
	generator.loadMem32(RDX, RDI, REG(instr.rA()));
	generator.andImm32(RDX, ~mask);
	generator.orReg32(RAX, RDX);
	generator.storeMem32(RDI, REG(instr.rA()), RAX);
	generateFlagsUpdate(instr.rc());
}

void PPCCodeGenerator::generateRlwinm(PPCInstruction instr) {
	uint32_t mask = genmask(instr.mb(), instr.me());
	
	generator.loadMem32(RAX, RDI, REG(instr.rS()));
	generator.rolImm32(RAX, instr.sh());
	generator.andImm32(RAX, mask);
	generator.storeMem32(RDI, REG(instr.rA()), RAX);
	generateFlagsUpdate(instr.rc());
}

void PPCCodeGenerator::generateOri(PPCInstruction instr) {
	generator.loadMem32(RAX, RDI, REG(instr.rS()));
	generator.orImm32(RAX, instr.uimm());
	generator.storeMem32(RDI, REG(instr.rA()), RAX);
	generator.ret();
}

void PPCCodeGenerator::generateOris(PPCInstruction instr) {
	generator.loadMem32(RAX, RDI, REG(instr.rS()));
	generator.orImm32(RAX, instr.uimm() << 16);
	generator.storeMem32(RDI, REG(instr.rA()), RAX);
	generator.ret();
}

void PPCCodeGenerator::generateXori(PPCInstruction instr) {
	generator.loadMem32(RAX, RDI, REG(instr.rS()));
	generator.xorImm32(RAX, instr.uimm());
	generator.storeMem32(RDI, REG(instr.rA()), RAX);
	generator.ret();
}

void PPCCodeGenerator::generateXoris(PPCInstruction instr) {
	generator.loadMem32(RAX, RDI, REG(instr.rS()));
	generator.xorImm32(RAX, instr.uimm() << 16);
	generator.storeMem32(RDI, REG(instr.rA()), RAX);
	generator.ret();
}

void PPCCodeGenerator::generateAndi(PPCInstruction instr) {
	generator.loadMem32(RAX, RDI, REG(instr.rS()));
	generator.andImm32(RAX, instr.uimm());
	generator.storeMem32(RDI, REG(instr.rA()), RAX);
	generateFlagsUpdate(true);
}

void PPCCodeGenerator::generateAndis(PPCInstruction instr) {
	generator.loadMem32(RAX, RDI, REG(instr.rS()));
	generator.andImm32(RAX, instr.uimm() << 16);
	generator.storeMem32(RDI, REG(instr.rA()), RAX);
	generateFlagsUpdate(true);
}

void PPCCodeGenerator::generateAdd(PPCInstruction instr) {
	generator.loadMem32(RAX, RDI, REG(instr.rA()));
	generator.addRegMem32(RAX, RDI, REG(instr.rB()));
	generator.storeMem32(RDI, REG(instr.rD()), RAX);
	generateFlagsUpdate(instr.rc());
}

void PPCCodeGenerator::generateLwbrx(PPCInstruction instr) {
	generator.pushReg64(RDI);
	if (instr.rA()) {
		generator.loadMem32(RSI, RDI, REG(instr.rA()));
		generator.addRegMem32(RSI, RDI, REG(instr.rB()));
	}
	else {
		generator.loadMem32(RSI, RDI, REG(instr.rB()));
	}
	generator.lea64(RDX, RDI, REG(instr.rD()));
	generator.callAbs(RAX, (uint64_t)loadMemory<uint32_t>);
	generator.popReg64(RDI);
	generator.testReg32(RAX, RAX);
	generator.jumpIfNotZero(1);
	generator.ret();
	generator.loadMem32(RAX, RDI, REG(instr.rD()));
	generator.swap32(RAX);
	generator.storeMem32(RDI, REG(instr.rD()), RAX);
	generator.ret();
}

void PPCCodeGenerator::generateStwbrx(PPCInstruction instr) {
	if (instr.rA()) {
		generator.loadMem32(RSI, RDI, REG(instr.rA()));
		generator.addRegMem32(RSI, RDI, REG(instr.rB()));
	}
	else {
		generator.loadMem32(RSI, RDI, REG(instr.rB()));
	}
	generator.loadMem32(RDX, RDI, REG(instr.rS()));
	generator.swap32(RDX);
	generator.jumpAbs(RAX, (uint64_t)storeMemory<uint32_t>);
}

void PPCCodeGenerator::generateLfsx(PPCInstruction instr) {
	if (instr.rA()) {
		generator.loadMem32(RSI, RDI, REG(instr.rA()));
		generator.addRegMem32(RSI, RDI, REG(instr.rB()));
	}
	else {
		generator.loadMem32(RSI, RDI, REG(instr.rB()));
	}
	generator.lea64(RDX, RDI, FPR_PS0(instr.rD()));
	generator.jumpAbs(RAX, (uint64_t)loadMemory<float>);
}

void PPCCodeGenerator::generateLfdx(PPCInstruction instr) {
	if (instr.rA()) {
		generator.loadMem32(RSI, RDI, REG(instr.rA()));
		generator.addRegMem32(RSI, RDI, REG(instr.rB()));
	}
	else {
		generator.loadMem32(RSI, RDI, REG(instr.rB()));
	}
	generator.lea64(RDX, RDI, FPR_DBL(instr.rD()));
	generator.jumpAbs(RAX, (uint64_t)loadMemory<double>);
}

void PPCCodeGenerator::generateStfsx(PPCInstruction instr) {
	if (instr.rA()) {
		generator.loadMem32(RSI, RDI, REG(instr.rA()));
		generator.addRegMem32(RSI, RDI, REG(instr.rB()));
	}
	else {
		generator.loadMem32(RSI, RDI, REG(instr.rB()));
	}
	generator.loadMem32(RDX, RDI, FPR_PS0(instr.rS()));
	generator.jumpAbs(RAX, (uint64_t)storeMemory<float>);
}

void PPCCodeGenerator::generateStfdx(PPCInstruction instr) {
	if (instr.rA()) {
		generator.loadMem32(RSI, RDI, REG(instr.rA()));
		generator.addRegMem32(RSI, RDI, REG(instr.rB()));
	}
	else {
		generator.loadMem32(RSI, RDI, REG(instr.rB()));
	}
	generator.loadMem64(RDX, RDI, FPR_DBL(instr.rS()));
	generator.jumpAbs(RAX, (uint64_t)storeLong);
}

void PPCCodeGenerator::generateStfiwx(PPCInstruction instr) {
	if (instr.rA()) {
		generator.loadMem32(RSI, RDI, REG(instr.rA()));
		generator.addRegMem32(RSI, RDI, REG(instr.rB()));
	}
	else {
		generator.loadMem32(RSI, RDI, REG(instr.rB()));
	}
	generator.loadMem32(RDX, RDI, FPR_IW1(instr.rS()));
	generator.jumpAbs(RAX, (uint64_t)storeMemory<uint32_t>);
}

void PPCCodeGenerator::generateFlagsUpdate(bool rc) {
	if (rc) {
		generator.jumpIfSign(28);
		generator.jumpIfZero(13); // 2 bytes
		
		// ~N & ~Z
		generator.andMemImm32(RDI, CR, 0x5FFFFFFF); // 7 bytes
		generator.bitTestSetMem32(RDI, CR, 30); // 5 bytes
		generator.ret(); // 1 byte
		
		// ~N & Z
		generator.andMemImm32(RDI, CR, 0x3FFFFFFF);
		generator.bitTestSetMem32(RDI, CR, 29);
		generator.ret();
		
		generator.jumpIfZero(13);
		
		// N & ~Z
		generator.andMemImm32(RDI, CR, 0x9FFFFFFF);
		generator.bitTestSetMem32(RDI, CR, 31);
		generator.ret();
		
		// N & Z
		generator.orMemImm32(RDI, CR, 0xA0000000);
		generator.bitTestResetMem32(RDI, CR, 30);
	}
	generator.ret();
}


PPCJIT::PPCJIT(PhysicalMemory *physmem, PPCProcessor *cpu) {
	this->physmem = physmem;
	this->cpu = cpu;
	
	memset(table, 0, sizeof(table));
	memset(sizes, 0, sizeof(sizes));
}

void PPCJIT::reset() {
	invalidate();
	
	#if STATS
	instrsCompiled = 0;
	instrsExecuted = 0;
	#endif
}

void PPCJIT::invalidate() {
	for (int i = 0; i < 0x100000; i++) {
		if (table[i]) {
			munmap(table[i], sizes[i]);
			table[i] = nullptr;
			sizes[i] = 0;
			
			#if STATS
			blocksInvalidated++;
			#endif
		}
	}
	
	#if STATS
	instrSize = 0;
	#endif
}

void PPCJIT::invalidateBlock(uint32_t addr) {
	int index = addr >> 12;
	if (table[index]) {
		#if STATS
		instrSize -= sizes[index];
		blocksInvalidated++;
		#endif
		
		munmap(table[index], sizes[index]);
		table[index] = nullptr;
		sizes[index] = 0;
	}
}

void PPCJIT::execute(uint32_t pc) {
	char *target = table[pc >> 12];
	if (!target) {
		target = generateCode(pc & ~0xFFF);
		table[pc >> 12] = target;
	}
	
	target = target + 5 * ((pc & 0xFFF) >> 2);
	
	#if STATS
	instrsExecuted++;
	#endif
	
	cpu->core.pc += 4;
	((JITEntryFunc)target)(cpu);
}

char *PPCJIT::generateCode(uint32_t pc) {
	PPCCodeGenerator generator;
	for (int i = 0; i < 0x400; i++) {
		PPCInstruction instr;
		instr.value = physmem->read<uint32_t>(pc + i * 4);
		generator.generate(instr);
	}
	
	#if STATS
	instrsCompiled += 0x400;
	instrSize += generator.size();
	#endif
	
	char *buffer = generator.get();
	size_t size = generator.size();
	
	sizes[pc >> 12] = size;
	
	char *jit = (char *)mmap(
		NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC,
		MAP_ANONYMOUS | MAP_SHARED, -1, 0
	);
	memcpy(jit, buffer, size);
	return jit;
}
