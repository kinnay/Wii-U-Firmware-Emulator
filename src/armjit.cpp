
#include "armjit.h"
#include "arminstruction.h"
#include "armprocessor.h"
#include "x86codegenerator.h"

#include "common/exceptions.h"

#include <sys/mman.h>

#include <cstring>


static const int CPSR = offsetof(ARMProcessor, core.cpsr);
static_assert(CPSR < 0x80, "offsetof core.cpsr < 0x80");

#define REG(i) offsetof(ARMProcessor, core.regs[ARMCore::i])
#define GPR(exp) offsetof(ARMProcessor, core.regs) + 4 * (exp)


static void executeInstr(ARMProcessor *cpu, ARMInstruction instr) {
	#if STATS
	cpu->jit.instrsExecuted--;
	#endif
	
	instr.execute(cpu);
}

static void throwInstr(ARMInstruction instr) {
	runtime_error("ARM instruction: 0x%08X", instr.value);
}

static void undefinedException(ARMProcessor *cpu) {
	cpu->core.triggerException(ARMCore::UndefinedInstruction);
}

static void softwareInterrupt(ARMProcessor *cpu) {
	cpu->core.triggerException(ARMCore::SoftwareInterrupt);
}

static void writeModeRegs(ARMProcessor *cpu) {
	cpu->core.writeModeRegs();
}

static void readModeRegs(ARMProcessor *cpu) {
	cpu->core.readModeRegs();
}

static void changeMode(ARMProcessor *cpu) {
	cpu->core.writeModeRegs();
	cpu->core.cpsr = cpu->core.spsr;
	cpu->core.readModeRegs();
}

static void updateFlags(ARMProcessor *cpu, uint32_t result) {
	cpu->core.cpsr.set(ARMCore::Z, result == 0);
	cpu->core.cpsr.set(ARMCore::N, result >> 31);
}

template <class T>
static bool loadMemory(ARMProcessor *cpu, uint32_t addr, uint32_t *ptr) {
	T value;
	if (!cpu->read<T>(addr, &value)) {
		return false;
	}
	*ptr = value;
	return true;
}

template <class T>
static bool storeMemory(ARMProcessor *cpu, uint32_t addr, uint32_t value) {
	return cpu->write<T>(addr, value);
}


ARMCodeGenerator::ARMCodeGenerator() : generator(0x8000) {
	generator.seek(0x400 * 5);
	index = 0;
}

char *ARMCodeGenerator::get() {
	generator.seek(0);
	for (int i = 0; i < index; i++) {
		generator.jumpRel(offsets[i]);
	}
	return generator.get();
}

size_t ARMCodeGenerator::size() {
	return generator.size();
}

void ARMCodeGenerator::generate(ARMInstruction instr) {
	offsets[index] = generator.tell() - (index + 1) * 5;
	generateInstr(instr);
	index++;
}

void ARMCodeGenerator::generateInstr(ARMInstruction instr) {
	generateCondition(instr);
	
	if ((instr.value >> 28) == 0xF) {
		if (((instr.value >> 24) & 0xF) == 0xF) {
			generateUndefined();
		}
		else {
			generateError(instr);
		}
	}
	
	else {
		int type = (instr.value >> 25) & 7;
		if (type == 0) {
			if ((instr.value & 0x90) == 0x90) {
				if ((instr.value & 0x60) == 0) {
					if ((instr.value >> 24) & 1) {
						if ((instr.value >> 23) & 1) {
							generateError(instr);
						}
						else {
							generateUnimplemented(instr); // swap
						}
					}
					else {
						generateUnimplemented(instr); // multiply
					}
				}
				else {
					generateUnimplemented(instr); // load store extra
				}
			}
			else if ((instr.value & 0x01900000) == 0x01000000) {
				if (((instr.value >> 24) & 0xF) == 1) {
					if (instr.value & 0x80) {
						generateError(instr);
					}
					else {
						int type = (instr.value >> 4) & 7;
						if (type == 0) {
							if ((instr.value >> 21) & 1) {
								generateUnimplemented(instr); // move status register reg
							}
							else {
								generateReadStatusReg(instr);
							}
						}
						else if (type == 1) {
							if ((instr.value >> 22) & 1) {
								generateUnimplemented(instr); // count leading zeros
							}
							else {
								generateBranchExchange(instr);
							}
						}
						else if (type == 2) generateError(instr);
						else generateBranchLinkExchange(instr);
					}
				}
				else {
					generateError(instr);
				}
			}
			else {
				generateDataProcessingReg(instr);
			}
		}
		else if (type == 1) {
			if ((instr.value & 0x01900000) == 0x01000000) {
				if ((instr.value >> 21) & 1) {
					// move status register imm
					generateUnimplemented(instr);
				}
				else {
					generateUndefined();
				}
			}
			else {
				generateDataProcessingImm(instr);
			}
		}
		else if (type == 2) generateLoadStoreImm(instr);
		else if (type == 3) {
			if (instr.value & 0x10) {
				if (instr.value & 0x01F000F0) {
					generateUndefined();
				}
				else {
					generateError(instr);
				}
			}
			else {
				generateLoadStoreReg(instr);
			}
		}
		else if (type == 4) generateLoadStoreMultiple(instr);
		else if (type == 5) generateBranch(instr);
		else if (type == 6) generateError(instr);
		else if (type == 7) {
			if (instr.value & 0x01000000) generateSoftwareInterrupt();
			else if (instr.value & 0x10) generateUnimplemented(instr); // coprocessor transfer
			else {
				generateError(instr);
			}
		}
	}
}

void ARMCodeGenerator::generateCondition(ARMInstruction instr) {
	int type = instr.cond();
	if (type >= 14) return;
	
	if (type < 8) { // 7 bytes
		int flag = type >> 1;
		if (flag == 0) generator.bitTestMem32(RDI, CPSR, 30); // 5 bytes
		else if (flag == 1) generator.bitTestMem32(RDI, CPSR, 29); // 5 bytes
		else if (flag == 2) generator.bitTestMem32(RDI, CPSR, 31); // 5 bytes
		else if (flag == 3) generator.bitTestMem32(RDI, CPSR, 28); // 5 bytes
		
		if (type & 1) generator.jumpIfNotCarry(1); // 2 bytes
		else {
			generator.jumpIfCarry(1); // 2 bytes
		}
	}
	else if (type == 8) { // 14 bytes
		generator.bitTestMem32(RDI, CPSR, 29); // 5 bytes
		generator.jumpIfNotCarry(7); // 2 bytes
		generator.bitTestMem32(RDI, CPSR, 30); // 5 bytes
		generator.jumpIfNotCarry(1); // 2 bytes
	}
	else if (type == 9) { // 14 bytes
		generator.bitTestMem32(RDI, CPSR, 29); // 5 bytes
		generator.jumpIfNotCarry(8); // 2 bytes
		generator.bitTestMem32(RDI, CPSR, 30); // 5 bytes
		generator.jumpIfCarry(1); // 2 bytes
	}
	else { // 22 bytes
		generator.loadMem32(RAX, RDI, CPSR); // 3 bytes
		if (type == 12) { // 6 bytes
			generator.bitTestReg32(RAX, 30); // 4 bytes
			generator.jumpIfCarry(13); // 2 bytes
		}
		else if (type == 13) { // 6 bytes
			generator.bitTestReg32(RAX, 30); // 4 bytes
			generator.jumpIfCarry(14); // 2 bytes
		}
		generator.movReg32(RDX, RAX); // 2 bytes
		generator.shrImm32(RDX, 3); // 3 bytes
		generator.xorReg32(RAX, RDX); // 2 bytes
		generator.bitTestReg32(RAX, 28); // 4 bytes
		if (type & 1) {
			generator.jumpIfCarry(1); // 2 bytes
		}
		else {
			generator.jumpIfNotCarry(1); // 2 bytes
		}
	}
	
	generator.ret(); // 1 byte
}

void ARMCodeGenerator::generateUnimplemented(ARMInstruction instr) {
	generator.movImm32(RSI, instr.value);
	generator.jumpAbs(RAX, (uint64_t)executeInstr);
}

void ARMCodeGenerator::generateError(ARMInstruction instr) {
	generator.movImm32(RDI, instr.value);
	generator.jumpAbs(RAX, (uint64_t)throwInstr);
}

void ARMCodeGenerator::generateUndefined() {
	generator.jumpAbs(RAX, (uint64_t)undefinedException);
}

void ARMCodeGenerator::generateSoftwareInterrupt() {
	generator.jumpAbs(RAX, (uint64_t)softwareInterrupt);
}

void ARMCodeGenerator::generateReadReg(Register target, int reg) {
	generator.loadMem32(target, RDI, GPR(reg));
	if (reg == ARMCore::PC) {
		generator.addRegImm32(target, 4);
	}
}

void ARMCodeGenerator::generateReadShifted(Register target, ARMInstruction instr, bool s) {
	if (instr.shift() & 1) {
		generateReadShiftedReg(target, instr, s);
	}
	else {
		generateReadShiftedImm(target, instr, s);
	}
}

void ARMCodeGenerator::generateReadShiftedImm(Register target, ARMInstruction instr, bool s) {
	generateReadReg(target, instr.r3());
	
	int shift = instr.shift();
	int amount = shift >> 3;
	int type = (shift >> 1) & 3;
	
	if (type == 0) { // Logical shift left
		if (amount) {
			generator.shlImm32(target, amount);
			if (s) {
				generateCarryUpdate();
			}
		}
	}
	else if (type == 1) { // Logical shift right
		if (amount) {
			generator.shrImm32(target, amount);
			if (s) {
				generateCarryUpdate();
			}
		}
	}
	else if (type == 2) { // Arithmetic shift right
		if (amount == 0) {
			if (s) {
				generator.bitTestReg32(target, 31);
				generateCarryUpdate();
			}
			generator.sarImm32(target, 31);
		}
		else {
			generator.sarImm32(target, amount);
			if (s) {
				generateCarryUpdate();
			}
		}
	}
	else if (type == 3) { // Rotate right
		if (amount == 0) { // With extend
			generator.bitTestMem32(RDI, CPSR, 29);
			generator.rcrImm32(target, 1);
			if (s) {
				generateCarryUpdate();
			}
		}
		else {
			generator.rorImm32(target, amount);
			if (s) {
				generator.bitTestReg32(target, 31);
				generateCarryUpdate();
			}
		}
	}
}

void ARMCodeGenerator::generateReadShiftedReg(Register target, ARMInstruction instr, bool s) {
	generateReadReg(target, instr.r3());
	generateReadReg(RCX, instr.r2());
	
	int type = (instr.shift() >> 1) & 3;
	if (type == 0) { // Logical shift left
		generator.andImm32(RCX, 0xFF);
		generator.compareImm32(RCX, 32);
		if (s) {
			generator.jumpIfBelow(20);
			generator.movImm32(target, 0); // 5 bytes
			generator.jumpIfEqual(7); // 2 bytes
			generator.bitTestResetMem32(RDI, CPSR, 29); // 5 bytes
			generator.jumpRel(22); // 2 bytes
			generator.bitTestReg32(target, 0); // 4 bytes
			generator.jumpRel(2); // 2 bytes
			generator.shlReg32(target); // 2 bytes
			generateCarryUpdate(); // 14 bytes
		}
		else {
			generator.jumpIfBelow(7);
			generator.movImm32(target, 0); // 5 bytes
			generator.jumpRel(2);
			generator.shlReg32(target); // 2 bytes
		}
	}
	else if (type == 1) { // Logical shift right
		generator.andImm32(RCX, 0xFF);
		generator.compareImm32(RCX, 32);
		if (s) {
			generator.jumpIfBelow(20);
			generator.movImm32(target, 0); // 5 bytes
			generator.jumpIfEqual(7); // 2 bytes
			generator.bitTestResetMem32(RDI, CPSR, 29); // 5 bytes
			generator.jumpRel(22); // 2 bytes
			generator.bitTestReg32(target, 31); // 4 bytes
			generator.jumpRel(2); // 2 bytes
			generator.shrReg32(target); // 2 bytes
			generateCarryUpdate(); // 14 bytes
		}
		else {
			generator.jumpIfBelow(7);
			generator.movImm32(target, 0); // 5 bytes
			generator.jumpRel(2);
			generator.shrReg32(target); // 2 bytes
		}
	}
	else if (type == 2) { // Arithmetic shift right
		generator.andImm32(RCX, 0xFF);
		generator.compareImm32(RCX, 32);
		if (s) {
			generator.jumpIfBelow(13);
			generator.movImm32(RCX, 31); // 5 bytes
			generator.sarReg32(target); // 2 bytes
			generator.bitTestReg32(target, 0); // 4 bytes
			generator.jumpRel(2); // 2 bytes
			generator.sarReg32(target); // 2 bytes
			generateCarryUpdate(); // 14 bytes
		}
		else {
			generator.jumpIfBelow(5);
			generator.movImm32(RCX, 31); // 5 bytes
			generator.sarReg32(target);
		}
	}
	else { // Rotate right
		generator.rorReg32(target);
		if (s) {
			generator.bitTestReg32(target, 31);
			generateCarryUpdate();
		}
	}
}

void ARMCodeGenerator::generateBranch(ARMInstruction instr) {
	if (instr.link()) {
		generator.loadMem32(RAX, RDI, REG(PC));
		generator.storeMem32(RDI, REG(LR), RAX);
	}
	generator.addMemImm32(RDI, REG(PC), instr.offset() + 4);
	generator.ret();
}

void ARMCodeGenerator::generateBranchExchange(ARMInstruction instr) {
	generateReadReg(RAX, instr.r3());
	generator.movReg32(RDX, RAX);
	generator.andImm32(RAX, 1);
	generator.shlImm32(RAX, 5);
	generator.orMemReg32(RDI, CPSR, RAX);
	generator.andImm32(RDX, ~1);
	generator.storeMem32(RDI, REG(PC), RDX);
	generator.ret();
}

void ARMCodeGenerator::generateBranchLinkExchange(ARMInstruction instr) {
	generator.loadMem32(RAX, RDI, REG(PC));
	generator.storeMem32(RDI, REG(LR), RAX);
		
	generateReadReg(RAX, instr.r3());
	generator.movReg32(RDX, RAX);
	generator.andImm32(RAX, 1);
	generator.shlImm32(RAX, 5);
	generator.orMemReg32(RDI, CPSR, RAX);
	generator.andImm32(RDX, ~1);
	generator.storeMem32(RDI, REG(PC), RDX);
	generator.ret();
}

void ARMCodeGenerator::generateLoadStoreImm(ARMInstruction instr) {
	uint32_t offset = instr.value & 0xFFF;
	generator.movImm32(RAX, offset);
	
	if (instr.b()) {
		generateLoadStore<uint8_t>(instr, false);
	}
	else {
		generateLoadStore<uint32_t>(instr, true);
	}
}

void ARMCodeGenerator::generateLoadStoreReg(ARMInstruction instr) {
	generateReadShifted(RAX, instr, false);
	if (instr.b()) {
		generateLoadStore<uint8_t>(instr, false);
	}
	else {
		generateLoadStore<uint32_t>(instr, true);
	}
}

template <class T>
void ARMCodeGenerator::generateLoadStore(ARMInstruction instr, bool exchange) {
	generateReadReg(RSI, instr.r0());
	
	generator.movReg32(RCX, RSI);
	if (instr.u()) {
		generator.addRegReg32(RCX, RAX);
	}
	else {
		generator.subRegReg32(RCX, RAX);
	}
	
	if (instr.p()) {
		generator.movReg32(RSI, RCX);
	}
	
	bool writeBack = instr.w() || !instr.p();
	if (instr.r1() != ARMCore::PC) {
		exchange = false;
	}
	
	if (writeBack) {
		generator.pushReg64(RCX);
		generator.pushReg64(RDI);
		generator.pushReg64(RDI);
	}
	else if (exchange) {
		generator.pushReg64(RDI);
	}
	
	if (instr.l()) {
		generator.lea64(RDX, RDI, GPR(instr.r1()));
		if (!writeBack && !exchange) {
			generator.jumpAbs(RAX, (uint64_t)loadMemory<T>);
			return;
		}
		
		generator.callAbs(RAX, (uint64_t)loadMemory<T>);
		
		generator.popReg64(RDI);
		if (writeBack) {
			generator.popReg64(RDI);
			generator.popReg64(RCX);
		}
		
		generator.testReg32(RAX, RAX);
		generator.jumpIfNotZero(1);
		generator.ret();
		
		if (exchange) {
			generator.bitTestResetMem32(RDI, REG(PC), 0);
			generator.jumpIfNotCarry(5);
			generator.bitTestSetMem32(RDI, CPSR, 5);
		}
	}
	else {
		generator.loadMem32(RDX, RDI, GPR(instr.r1()));
		if (!writeBack) {
			generator.jumpAbs(RAX, (uint64_t)storeMemory<T>);
			return;
		}
		
		generator.callAbs(RAX, (uint64_t)storeMemory<T>);
		
		generator.popReg64(RDI);
		generator.popReg64(RDI);
		generator.popReg64(RCX);
		
		generator.testReg32(RAX, RAX);
		generator.jumpIfNotZero(1);
		generator.ret();
	}
	
	if (writeBack) {
		generator.storeMem32(RDI, GPR(instr.r0()), RCX);
	}
	
	generator.ret();
}

void ARMCodeGenerator::generateLoadStoreMultiple(ARMInstruction instr) {
	int adder = instr.u() ? 4 : -4;
	int reg = instr.u() ? 0 : 15;
	int inc = instr.u() ? 1 : -1;
	
	bool userMode = (instr.value & (1 << 22)) && (!(instr.value & (1 << 15)) || !instr.l());
	
	uint32_t regs = offsetof(ARMProcessor, core.regs);
	
	generator.pushReg64(RBP);
	generator.pushReg64(RBX);
	generator.pushReg64(RBX);
	
	generator.movReg64(RBP, RDI);
	generateReadReg(RBX, instr.r0());
	
	if (userMode) {
		generator.callAbs(RAX, (uint64_t)writeModeRegs);
		regs = offsetof(ARMProcessor, core.regsUser);
	}
	
	for (int i = 0; i < 16; i++) {
		if (instr.value & (1 << reg)) {
			if (instr.p()) {
				generator.addRegImm32(RBX, adder);
			}
			if (instr.l()) {
				generator.movReg64(RDI, RBP);
				generator.movReg32(RSI, RBX);
				generator.lea64(RDX, RDI, regs + 4 * reg);
				generator.callAbs(RAX, (uint64_t)loadMemory<uint32_t>);
				
				generator.testReg32(RAX, RAX);
				generator.jumpIfNotZero(4);
				generator.popReg64(RBX); // 1 byte
				generator.popReg64(RBX); // 1 byte
				generator.popReg64(RBP); // 1 byte
				generator.ret(); // 1 byte
				
				if (!userMode && reg == ARMCore::PC) {
					generator.bitTestResetMem32(RBP, regs + 4 * reg, 0); // 4 bytes
					generator.jumpIfNotCarry(5); // 2 bytes
					generator.bitTestSetMem32(RBP, CPSR, 5); // 5 bytes
				}
			}
			else {
				generator.movReg64(RDI, RBP);
				generator.movReg32(RSI, RBX);
				generator.loadMem32(RDX, RDI, regs + reg * 4);
				generator.callAbs(RAX, (uint64_t)storeMemory<uint32_t>);
				
				generator.testReg32(RAX, RAX);
				generator.jumpIfNotZero(4);
				generator.popReg64(RBX); // 1 byte
				generator.popReg64(RBX); // 1 byte
				generator.popReg64(RBP); // 1 byte
				generator.ret(); // 1 byte
			}
			if (!instr.p()) {
				generator.addRegImm32(RBX, adder); // 6 bytes
			}
		}
		
		reg += inc;
	}
	
	if (userMode) {
		generator.movReg64(RDI, RBP);
		generator.callAbs(RAX, (uint64_t)readModeRegs);
	}
	
	if (instr.w()) {
		generator.storeMem32(RBP, GPR(instr.r0()), RBX);
	}
	
	if (!userMode && instr.value & (1 << 22)) {
		generator.movReg64(RDI, RBP);
		generator.callAbs(RAX, (uint64_t)changeMode);
	}
	
	generator.popReg64(RBX);
	generator.popReg64(RBX);
	generator.popReg64(RBP);
	generator.ret();
}

void ARMCodeGenerator::generateDataProcessingImm(ARMInstruction instr) {
	int opcode = instr.opcode();
	
	int rot = instr.rotate() * 2;
	uint32_t imm = instr.value & 0xFF;
	if (rot) {
		bool shiftCarry = instr.s();
		if (opcode == 2 || opcode == 3 || opcode == 4 || opcode == 10 || opcode == 11) {
			shiftCarry = false;
		}
		
		imm = (imm >> rot) | (imm << (32 - rot));
		if (shiftCarry) {
			if (imm >> 31) {
				generator.bitTestSetMem32(RDI, CPSR, 29);
			}
			else {
				generator.bitTestResetMem32(RDI, CPSR, 29);
			}
		}
	}
	
	if (opcode == 3 || opcode == 7) {
		generateReadReg(RDX, instr.r0());
	}
	else if (opcode != 13 && opcode != 15) {
		generateReadReg(RAX, instr.r0());
	}
	
	if (opcode == 0 || opcode == 8) generator.andImm32(RAX, imm);
	else if (opcode == 1 || opcode == 9) generator.xorImm32(RAX, imm);
	else if (opcode == 2 || opcode == 10) generateSubImm(RAX, imm, instr.s());
	else if (opcode == 3) {
		generator.movImm32(RAX, imm);
		generateSubReg(RAX, RDX, instr.s());
	}
	else if (opcode == 4 || opcode == 11) generateAddImm(RAX, imm, instr.s());
	else if (opcode == 5) generateAddImmCarry(RAX, imm, instr.s());
	else if (opcode == 6) generateSubImmCarry(RAX, imm, instr.s());
	else if (opcode == 7) {
		generator.movImm32(RAX, imm);
		generateSubRegCarry(RAX, RDX, instr.s());
	}
	else if (opcode == 12) generator.orImm32(RAX, imm);
	else if (opcode == 13) generator.movImm32(RAX, imm);
	else if (opcode == 14) generator.andImm32(RAX, ~imm);
	else {
		generator.movImm32(RAX, ~imm);
	}
	
	if (opcode < 8 || opcode >= 12) {
		if (instr.r1() == ARMCore::PC) {
			generator.movReg32(RDX, RAX);
			generator.andImm32(RDX, 1);
			generator.shlImm32(RDX, 5);
			generator.orMemReg32(RDI, CPSR, RDX);
			generator.andImm32(RAX, ~1);
		}
		generator.storeMem32(RDI, GPR(instr.r1()), RAX);
	}
	
	if (instr.s()) {
		if (instr.r1() == ARMCore::PC) {
			generator.jumpAbs(RAX, (uint64_t)changeMode);
		}
		else {
			generateFlagsUpdate(RAX);
			generator.ret();
		}
	}
	else {
		generator.ret();
	}
}

void ARMCodeGenerator::generateDataProcessingReg(ARMInstruction instr) {
	int opcode = instr.opcode();
	
	if (opcode == 3 || opcode == 7) {
		generateReadShifted(RAX, instr, false);
		generateReadReg(RDX, instr.r0());
	}
	else if (opcode == 13 || opcode == 15) {
		generateReadShifted(RAX, instr, instr.s());
	}
	else {
		bool shiftCarry = instr.s();
		if (opcode == 2 || opcode == 3 || opcode == 4 || opcode == 10 || opcode == 11) {
			shiftCarry = false;
		}
		
		generateReadShifted(RDX, instr, shiftCarry);
		generateReadReg(RAX, instr.r0());
	}
	
	if (opcode == 0 || opcode == 8) generator.andReg32(RAX, RDX);
	else if (opcode == 1 || opcode == 9) generator.xorReg32(RAX, RDX);
	else if (opcode == 2 || opcode == 3 || opcode == 10) generateSubReg(RAX, RDX, instr.s());
	else if (opcode == 4 || opcode == 11) generateAddReg(RAX, RDX, instr.s());
	else if (opcode == 5) generateAddRegCarry(RAX, RDX, instr.s());
	else if (opcode == 6 || opcode == 7) generateSubRegCarry(RAX, RDX, instr.s());
	else if (opcode == 12) generator.orReg32(RAX, RDX);
	else if (opcode == 14) {
		generator.notReg32(RDX);
		generator.andReg32(RAX, RDX);
	}
	else if (opcode == 15) {
		generator.notReg32(RAX);
	}
	
	if (opcode < 8 || opcode >= 12) {
		if (instr.r1() == ARMCore::PC) {
			generator.movReg32(RDX, RAX);
			generator.andImm32(RDX, 1);
			generator.shlImm32(RDX, 5);
			generator.orMemReg32(RDI, CPSR, RDX);
			generator.andImm32(RAX, ~1);
		}
		generator.storeMem32(RDI, GPR(instr.r1()), RAX);
	}
	
	if (instr.s()) {
		if (instr.r1() == ARMCore::PC) {
			generator.jumpAbs(RAX, (uint64_t)changeMode);
		}
		else {
			generateFlagsUpdate(RAX);
			generator.ret();
		}
	}
	else {
		generator.ret();
	}
}

void ARMCodeGenerator::generateAddImm(Register reg, uint32_t imm, bool s) {
	generator.addRegImm32(reg, imm);
	if (s) {
		generateOverflowUpdate();
	}
}

void ARMCodeGenerator::generateAddImmCarry(Register reg, uint32_t imm, bool s) {
	generator.bitTestMem32(RDI, CPSR, 29);
	generator.adcRegImm32(reg, imm);
	if (s) {
		generateOverflowUpdate();
	}
}

void ARMCodeGenerator::generateAddReg(Register reg, Register other, bool s) {
	generator.addRegReg32(reg, other);
	if (s) {
		generateOverflowUpdate();
	}
}

void ARMCodeGenerator::generateAddRegCarry(Register reg, Register other, bool s) {
	generator.bitTestMem32(RDI, CPSR, 29);
	generator.adcRegReg32(reg, other);
	if (s) {
		generateOverflowUpdate();
	}
}

void ARMCodeGenerator::generateSubImm(Register reg, uint32_t imm, bool s) {
	generator.subRegImm32(reg, imm);
	if (s) {
		generateOverflowUpdateInv();
	}
}

void ARMCodeGenerator::generateSubImmCarry(Register reg, uint32_t imm, bool s) {
	generator.bitTestMem32(RDI, CPSR, 29);
	generator.flipCarry();
	
	generator.sbbRegImm32(reg, imm);
	if (s) {
		generateOverflowUpdateInv();
	}
}

void ARMCodeGenerator::generateSubReg(Register reg, Register other, bool s) {
	generator.subRegReg32(reg, other);
	if (s) {
		generateOverflowUpdateInv();
	}
}

void ARMCodeGenerator::generateSubRegCarry(Register reg, Register other, bool s) {
	generator.bitTestMem32(RDI, CPSR, 29);
	generator.flipCarry();
	
	generator.sbbRegReg32(reg, other);
	if (s) {
		generateOverflowUpdateInv();
	}
}

void ARMCodeGenerator::generateCarryUpdate() {
	generator.jumpIfCarry(7); // 2 bytes
	generator.bitTestResetMem32(RDI, CPSR, 29); // 5 bytes
	generator.jumpRel(5); // 2 bytes
	generator.bitTestSetMem32(RDI, CPSR, 29); // 5 bytes
}

void ARMCodeGenerator::generateOverflowUpdate() {
	generator.jumpIfCarry(23); // 2 bytes
	generator.jumpIfOverflow(9); // 2 bytes
	
	// 9 bytes (~C & ~V)
	generator.andMemImm32(RDI, CPSR, 0xCFFFFFFF);
	generator.jumpRel(33);
	
	// 12 bytes (~C & V)
	generator.bitTestResetMem32(RDI, CPSR, 29);
	generator.bitTestSetMem32(RDI, CPSR, 28);
	generator.jumpRel(21);
	
	generator.jumpIfOverflow(12); // 2 bytes
	
	// 12 bytes (C & ~V)
	generator.bitTestSetMem32(RDI, CPSR, 29);
	generator.bitTestResetMem32(RDI, CPSR, 28);
	generator.jumpRel(7);
	
	// 7 bytes (C & V)
	generator.orMemImm32(RDI, CPSR, 0x30000000);
}

void ARMCodeGenerator::generateOverflowUpdateInv() {
	generator.jumpIfNotCarry(23); // 2 bytes
	generator.jumpIfOverflow(9); // 2 bytes
	
	// 9 bytes (~C & ~V)
	generator.andMemImm32(RDI, CPSR, 0xCFFFFFFF);
	generator.jumpRel(33);
	
	// 12 bytes (~C & V)
	generator.bitTestResetMem32(RDI, CPSR, 29);
	generator.bitTestSetMem32(RDI, CPSR, 28);
	generator.jumpRel(21);
	
	generator.jumpIfOverflow(12); // 2 bytes
	
	// 12 bytes (C & ~V)
	generator.bitTestSetMem32(RDI, CPSR, 29);
	generator.bitTestResetMem32(RDI, CPSR, 28);
	generator.jumpRel(7);
	
	// 7 bytes (C & V)
	generator.orMemImm32(RDI, CPSR, 0x30000000);
}

void ARMCodeGenerator::generateFlagsUpdate(Register reg) {
	generator.andMemImm32(RDI, CPSR, 0x3FFFFFFF); // 7 bytes
	generator.testReg32(reg, reg); // 2 bytes
	generator.jumpIfNotZero(5); // 2 bytes
	generator.bitTestSetMem32(RDI, CPSR, 30); // 5 bytes
	generator.bitTestReg32(reg, 31); // 4 bytes
	generator.jumpIfNotCarry(5); // 2 bytes
	generator.bitTestSetMem32(RDI, CPSR, 31); // 5 bytes
}

void ARMCodeGenerator::generateReadStatusReg(ARMInstruction instr) {
	int rd = instr.r1();
	int offs = instr.r() ? offsetof(ARMProcessor, core.spsr) : CPSR;
	generator.loadMem32(RAX, RDI, offs);
	generator.storeMem32(RDI, GPR(rd), RAX);
	generator.ret();
}


ARMJIT::ARMJIT(PhysicalMemory *physmem, ARMProcessor *cpu) {
	this->physmem = physmem;
	this->cpu = cpu;
	
	memset(table, 0, sizeof(table));
	memset(sizes, 0, sizeof(sizes));
}

void ARMJIT::reset() {
	invalidate();
	
	#if STATS
	instrsCompiled = 0;
	instrsExecuted = 0;
	#endif
}

void ARMJIT::invalidate() {
	for (int i = 0; i < 0x100000; i++) {
		if (table[i]) {
			munmap(table[i], sizes[i]);
			table[i] = nullptr;
			sizes[i] = 0;
		}
	}
	
	#if STATS
	instrSize = 0;
	#endif
}

void ARMJIT::execute(uint32_t pc) {
	char *target = table[pc >> 12];
	if (!target) {
		target = generateCode(pc & ~0xFFF);
		table[pc >> 12] = target;
	}
	
	target = target + 5 * ((pc & 0xFFF) >> 2);
	
	#if STATS
	instrsExecuted++;
	#endif
	
	cpu->core.regs[ARMCore::PC] += 4;
	((JITEntryFunc)target)(cpu);
}

char *ARMJIT::generateCode(uint32_t pc) {
	ARMCodeGenerator generator;
	for (int i = 0; i < 0x400; i++) {
		ARMInstruction instr;
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
