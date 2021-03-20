
#include "cpu/arm/armthumbgenerator.h"
#include "cpu/arm/armthumbinstr.h"
#include "cpu/arm/armprocessor.h"
#include "cpu/x86codegenerator.h"

#include "common/exceptions.h"

#include <sys/mman.h>

#include <cstring>

#define CPSR offsetof(ARMProcessor, core.cpsr)
#define REG(i) offsetof(ARMProcessor, core.regs[ARMCore::i])
#define GPR(exp) offsetof(ARMProcessor, core.regs) + 4 * (exp)


static void undefinedInstr(ARMProcessor *cpu) {
	cpu->core.triggerException(ARMCore::UndefinedInstruction);
}

static void softwareInterrupt(ARMProcessor *cpu) {
	cpu->core.triggerException(ARMCore::SoftwareInterrupt);
}

static void throwInstr(ARMThumbInstr instr) {
	runtime_error("ARM thumb instruction: 0x%04X", instr.value);
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


ARMThumbGenerator::ARMThumbGenerator() : generator(0x8000) {
	generator.seek(0x800 * 5);
	index = 0;
}

char *ARMThumbGenerator::get() {
	generator.seek(0);
	for (int i = 0; i < index; i++) {
		generator.jumpRel(offsets[i]);
	}
	return generator.get();
}

size_t ARMThumbGenerator::size() {
	return generator.size();
}

void ARMThumbGenerator::generate(uint16_t value) {
	ARMThumbInstr instr;
	instr.value = value;
	
	offsets[index] = generator.tell() - (index + 1) * 5;
	generateInstr(instr);
	index++;
}

void ARMThumbGenerator::generateInstr(ARMThumbInstr instr) {
	if (!(instr.value >> 13)) {
		if (((instr.value >> 11) & 3) == 3) generateAddSubtract(instr);
		else generateMoveShifted(instr);
	}
	else if ((instr.value >> 13) == 1) generateAddSubCmpMovImm(instr);
	else if ((instr.value >> 10) == 0x10) generateDataProcessing(instr);
	else if ((instr.value >> 8) == 0x47) generateBranchExchange(instr);
	else if ((instr.value >> 10) == 0x11) generateSpecialDataProcessing(instr);
	else if ((instr.value >> 11) == 9) generateLoadPCRelative(instr);
	else if ((instr.value >> 12) == 5) generateLoadStoreRegOffs(instr);
	else if ((instr.value >> 13) == 3) generateLoadStoreImmOffs(instr);
	else if ((instr.value >> 12) == 8) generateLoadStoreHalf(instr);
	else if ((instr.value >> 12) == 9) generateLoadStoreStack(instr);
	else if ((instr.value >> 12) == 10) generateAddToSPOrPC(instr);
	else if ((instr.value >> 12) == 11) {
		if (((instr.value >> 8) & 0xF) == 0) generateAdjustStackPointer(instr);
		else if (((instr.value >> 9) & 3) == 2) generatePushPopRegisterList(instr);
		else generateError(instr);
	}
	else if ((instr.value >> 12) == 12) generateLoadStoreMultiple(instr);
	else if ((instr.value >> 12) == 13) {
		if (((instr.value >> 8) & 0xF) == 14) generateUndefined();
		else if (((instr.value >> 8) & 0xF) == 15) generateSoftwareInterrupt();
		else generateConditionalBranch(instr);
	}
	else if ((instr.value >> 11) == 0x1C) generateUnconditionalBranch(instr);
	else if ((instr.value >> 11) == 0x1D) generateUndefined();
	else if ((instr.value >> 12) == 0xF) generateLongBranchWithLink(instr);
	else {
		generateError(instr);
	}
}

void ARMThumbGenerator::generateError(ARMThumbInstr instr) {
	generator.movImm32(RDI, instr.value);
	generator.jumpAbs(RAX, (uint64_t)throwInstr);
}

void ARMThumbGenerator::generateUndefined() {
	generator.jumpAbs(RAX, (uint64_t)undefinedInstr);
}

void ARMThumbGenerator::generateSoftwareInterrupt() {
	generator.jumpAbs(RAX, (uint64_t)softwareInterrupt);
}

void ARMThumbGenerator::generateMoveShifted(ARMThumbInstr instr) {
	int imm = (instr.value >> 6) & 0x1F;
	int opcode = (instr.value >> 11) & 3;
	
	generator.loadMem32(RAX, RDI, GPR((instr.value >> 3) & 7));
	if (opcode == 0) generator.shlImm32(RAX, imm);
	else if (opcode == 1) generator.shrImm32(RAX, imm);
	else if (opcode == 2) generator.sarImm32(RAX, imm);
	
	generator.storeMem32(RDI, GPR(instr.value & 7), RAX);
	
	generateFlagsUpdate(RAX);
	
	generator.ret();
}

void ARMThumbGenerator::generateAddSubtract(ARMThumbInstr instr) {
	generator.loadMem32(RAX, RDI, GPR((instr.value >> 3) & 7));
	
	int rd = instr.value & 7;
	uint32_t imm = (instr.value >> 6) & 7;
	if (instr.i()) {
		if (instr.value & 0x200) {
			generator.subRegImm32(RAX, imm);
			generateOverflowUpdateInv();
		}
		else {
			generator.addRegImm32(RAX, imm);
			generateOverflowUpdate();
		}
	}
	else {
		generator.loadMem32(RDX, RDI, GPR(imm));
		if (instr.value & 0x200) {
			generator.subRegReg32(RAX, RDX);
			generateOverflowUpdateInv();
		}
		else {
			generator.addRegReg32(RAX, RDX);
			generateOverflowUpdate();
		}
	}
	
	generateFlagsUpdate(RAX);
	generator.storeMem32(RDI, GPR(rd), RAX);
	generator.ret();
}

void ARMThumbGenerator::generateAddSubCmpMovImm(ARMThumbInstr instr) {
	int reg = (instr.value >> 8) & 7;
	int imm = instr.value & 0xFF;
	
	int opcode = (instr.value >> 11) & 3;
	if (opcode == 0) {
		generator.storeMemImm32(RDI, GPR(reg), imm);
		if (imm == 0) {
			generator.andMemImm32(RDI, CPSR, 0x3FFFFFFF);
		}
		else {
			generator.bitTestSetMem32(RDI, CPSR, 30);
			generator.bitTestResetMem32(RDI, CPSR, 31);
		}
	}
	else if (opcode == 1) {
		generator.loadMem32(RAX, RDI, GPR(reg));
		generator.subRegImm32(RAX, imm);
		generateOverflowUpdateInv();
		generateFlagsUpdate(RAX);
	}
	else if (opcode == 2) {
		generator.loadMem32(RAX, RDI, GPR(reg));
		generator.addRegImm32(RAX, imm);
		generateOverflowUpdate();
		generateFlagsUpdate(RAX);
		generator.storeMem32(RDI, GPR(reg), RAX);
	}
	else {
		generator.loadMem32(RAX, RDI, GPR(reg));
		generator.subRegImm32(RAX, imm);
		generateOverflowUpdateInv();
		generateFlagsUpdate(RAX);
		generator.storeMem32(RDI, GPR(reg), RAX);
	}
	generator.ret();
}

void ARMThumbGenerator::generateSpecialDataProcessing(ARMThumbInstr instr) {
	int reg1 = (instr.value & 7) + ((instr.value >> 4) & 8);
	int reg2 = (instr.value >> 3) & 0xF;
	
	generateReadReg(RAX, reg2);
	
	int opcode = (instr.value >> 8) & 3;
	if (opcode == 0) {
		generator.addMemReg32(RDI, GPR(reg1), RAX);
	}
	else if (opcode == 1) {
		generator.loadMem32(RDX, RDI, GPR(reg1));
		generateSubReg(RDX, RAX);
		generateFlagsUpdate(RDX);
	}
	else {
		generator.storeMem32(RDI, GPR(reg1), RAX);
	}
	
	generator.ret();
}

void ARMThumbGenerator::generateDataProcessing(ARMThumbInstr instr) {
	int opcode = (instr.value >> 6) & 0xF;
	int destreg = instr.value & 7;
	
	if (opcode != 9 && opcode != 15) {
		generator.loadMem32(RAX, RDI, GPR(destreg));
		generator.loadMem32(RCX, RDI, GPR((instr.value >> 3) & 7));
	}
	else {
		generator.loadMem32(RAX, RDI, GPR((instr.value >> 3) & 7));
	}
	
	if (opcode == 0 || opcode == 8) generator.andReg32(RAX, RCX);
	else if (opcode == 1) generator.xorReg32(RAX, RCX);
	else if (opcode == 2) generator.shlReg32(RAX);
	else if (opcode == 3) generator.shrReg32(RAX);
	else if (opcode == 4) generator.sarReg32(RAX);
	else if (opcode == 5) generateAddRegCarry(RAX, RCX);
	else if (opcode == 6) generateSubRegCarry(RAX, RCX);
	else if (opcode == 7) generator.rorReg32(RAX);
	else if (opcode == 9) generator.negReg32(RAX);
	else if (opcode == 10) generateSubReg(RAX, RCX);
	else if (opcode == 11) generateAddReg(RAX, RCX);
	else if (opcode == 12) generator.orReg32(RAX, RCX);
	else if (opcode == 13) generator.mulReg32(RCX);
	else if (opcode == 14) {
		generator.notReg32(RCX);
		generator.andReg32(RAX, RCX);
	
	}
	else {
		generator.notReg32(RAX);
	}
	
	if (opcode != 8 && opcode != 10 && opcode != 11) {
		generator.storeMem32(RDI, GPR(destreg), RAX);
	}
	
	generateFlagsUpdate(RAX);
	generator.ret();
}

void ARMThumbGenerator::generateUnconditionalBranch(ARMThumbInstr instr) {
	int offset = instr.value & 0x7FF;
	if (offset & 0x400) offset -= 0x800;
	
	offset = offset * 2 + 2;
	
	generator.addMemImm32(RDI, REG(PC), offset);
	generator.ret();
}

void ARMThumbGenerator::generateConditionalBranch(ARMThumbInstr instr) {
	int type = (instr.value >> 8) & 0xF;
	if (type < 14) {
		if (type < 8) { // 10 bytes
			int flag = type >> 1;
			if (flag == 0) generator.bitTestMem32(RDI, CPSR, 30); // 8 bytes
			else if (flag == 1) generator.bitTestMem32(RDI, CPSR, 29); // 8 bytes
			else if (flag == 2) generator.bitTestMem32(RDI, CPSR, 31); // 8 bytes
			else if (flag == 3) generator.bitTestMem32(RDI, CPSR, 28); // 8 bytes
			
			if (type & 1) generator.jumpIfNotCarry(1); // 2 bytes
			else {
				generator.jumpIfCarry(1); // 2 bytes
			}
		}
		else if (type == 8) { // 20 bytes
			generator.bitTestMem32(RDI, CPSR, 29); // 8 bytes
			generator.jumpIfNotCarry(10); // 2 bytes
			generator.bitTestMem32(RDI, CPSR, 30); // 8 bytes
			generator.jumpIfNotCarry(1); // 2 bytes
		}
		else if (type == 9) { // 20 bytes
			generator.bitTestMem32(RDI, CPSR, 29); // 8 bytes
			generator.jumpIfNotCarry(11); // 2 bytes
			generator.bitTestMem32(RDI, CPSR, 30); // 8 bytes
			generator.jumpIfCarry(1); // 2 bytes
		}
		else { // 25 bytes
			generator.loadMem32(RAX, RDI, CPSR); // 6 bytes
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
	
	int offset = instr.value & 0xFF;
	if (offset & 0x80) offset -= 0x100;
	generator.addMemImm32(RDI, REG(PC), offset * 2 + 2);
	generator.ret();
}

void ARMThumbGenerator::generateLongBranchWithLink(ARMThumbInstr instr) {
	uint32_t offset = instr.value & 0x7FF;
	if (instr.h()) { // Instruction 2
		generator.loadMem32(RAX, RDI, REG(LR));
		generator.addRegImm32(RAX, offset << 1);
		generator.loadMem32(RDX, RDI, REG(PC));
		generator.orImm32(RDX, 1);
		generator.storeMem32(RDI, REG(LR), RDX);
		generator.storeMem32(RDI, REG(PC), RAX);
	}
	else { // Instruction 1
		if (offset & 0x400) offset -= 0x800;
		generator.loadMem32(RAX, RDI, REG(PC));
		generator.addRegImm32(RAX, 2 + (offset << 12));
		generator.storeMem32(RDI, REG(LR), RAX);
	}
	generator.ret();
}

void ARMThumbGenerator::generateBranchExchange(ARMThumbInstr instr) {
	int reg = (instr.value >> 3) & 0xF;
	generateReadReg(RDX, reg);
	
	if ((instr.value >> 7) & 1) {
		generator.loadMem32(RAX, RDI, REG(PC));
		generator.orImm32(RAX, 1);
		generator.storeMem32(RDI, REG(LR), RAX);
	}
	
	generator.bitTestResetReg32(RDX, 0);
	generator.jumpIfCarry(8);
	generator.bitTestResetMem32(RDI, CPSR, 5);
	generator.storeMem32(RDI, REG(PC), RDX);
	generator.ret();
}

void ARMThumbGenerator::generateAdjustStackPointer(ARMThumbInstr instr) {
	int offset = (instr.value & 0x7F) * 4;
	if (instr.value & 0x80) offset = -offset;
	
	generator.addMemImm32(RDI, REG(SP), offset);
	generator.ret();
}

void ARMThumbGenerator::generateAddToSPOrPC(ARMThumbInstr instr) {
	uint32_t value = (instr.value & 0xFF) * 4;
	if (instr.value & (1 << 11)) {
		generator.loadMem32(RAX, RDI, REG(SP));
		generator.addRegImm32(RAX, value);
	}
	else {
		generator.loadMem32(RAX, RDI, REG(PC));
		generator.addRegImm32(RAX, value + 2);
	}
	generator.storeMem32(RDI, GPR((instr.value >> 8) & 7), RAX);
	generator.ret();
}

void ARMThumbGenerator::generateLoadPCRelative(ARMThumbInstr instr) {
	uint32_t offset = (instr.value & 0xFF) * 4;
	generator.loadMem32(RSI, RDI, REG(PC));
	generator.addRegImm32(RSI, offset + 2);
	generator.andImm32(RSI, ~3);
	generator.lea64(RDX, RDI, GPR((instr.value >> 8) & 7));
	generator.jumpAbs(RAX, (uint64_t)loadMemory<uint32_t>);
}

void ARMThumbGenerator::generateLoadStoreStack(ARMThumbInstr instr) {
	int reg = (instr.value >> 8) & 7;
	uint32_t offset = (instr.value & 0xFF) * 4;
	
	generator.loadMem32(RSI, RDI, REG(SP));
	generator.addRegImm32(RSI, offset);
	if (instr.l()) {
		generator.lea64(RDX, RDI, GPR(reg));
		generator.jumpAbs(RAX, (uint64_t)loadMemory<uint32_t>);
	}
	else {
		generator.loadMem32(RDX, RDI, GPR(reg));
		generator.jumpAbs(RAX, (uint64_t)storeMemory<uint32_t>);
	}
}

void ARMThumbGenerator::generateLoadStoreHalf(ARMThumbInstr instr) {
	uint32_t offset = (instr.value >> 5) & 0x3E;
	
	generator.loadMem32(RSI, RDI, GPR((instr.value >> 3) & 7));
	generator.addRegImm32(RSI, offset);
	if (instr.l()) {
		generator.lea64(RDX, RDI, GPR(instr.value & 7));
		generator.jumpAbs(RAX, (uint64_t)loadMemory<uint16_t>);
	}
	else {
		generator.loadMem32(RDX, RDI, GPR(instr.value & 7));
		generator.jumpAbs(RAX, (uint64_t)storeMemory<uint16_t>);
	}
}

void ARMThumbGenerator::generateLoadStoreRegOffs(ARMThumbInstr instr) {
	int reg = instr.value & 7;
	int opcode = (instr.value >> 9) & 7;
	
	generator.loadMem32(RSI, RDI, GPR((instr.value >> 3) & 7));
	generator.addRegMem32(RSI, RDI, GPR((instr.value >> 6) & 7));
	
	if (opcode == 0) {
		generator.loadMem32(RDX, RDI, GPR(reg));
		generator.jumpAbs(RAX, (uint64_t)storeMemory<uint32_t>);
	}
	else if (opcode == 1) {
		generator.loadMem32(RDX, RDI, GPR(reg));
		generator.jumpAbs(RAX, (uint64_t)storeMemory<uint16_t>);
	}
	else if (opcode == 2) {
		generator.loadMem32(RDX, RDI, GPR(reg));
		generator.jumpAbs(RAX, (uint64_t)storeMemory<uint8_t>);
	}
	else if (opcode == 3) {
		generator.lea64(RDX, RDI, GPR(reg));
		generator.jumpAbs(RAX, (uint64_t)loadMemory<int8_t>);
	}
	else if (opcode == 4) {
		generator.lea64(RDX, RDI, GPR(reg));
		generator.jumpAbs(RAX, (uint64_t)loadMemory<uint32_t>);
	}
	else if (opcode == 5) {
		generator.lea64(RDX, RDI, GPR(reg));
		generator.jumpAbs(RAX, (uint64_t)loadMemory<uint16_t>);
	}
	else if (opcode == 6) {
		generator.lea64(RDX, RDI, GPR(reg));
		generator.jumpAbs(RAX, (uint64_t)loadMemory<uint8_t>);
	}
	else {
		generator.lea64(RDX, RDI, GPR(reg));
		generator.jumpAbs(RAX, (uint64_t)loadMemory<int16_t>);
	}
}

void ARMThumbGenerator::generateLoadStoreImmOffs(ARMThumbInstr instr) {
	int reg = instr.value & 7;
	uint32_t offset = (instr.value >> 6) & 0x1F;
	
	generator.loadMem32(RSI, RDI, GPR((instr.value >> 3) & 7));
	if (instr.b()) {
		generator.addRegImm32(RSI, offset);
		if (instr.l()) {
			generator.lea64(RDX, RDI, GPR(reg));
			generator.jumpAbs(RAX, (uint64_t)loadMemory<uint8_t>);
		}
		else {
			generator.loadMem32(RDX, RDI, GPR(reg));
			generator.jumpAbs(RAX, (uint64_t)storeMemory<uint8_t>);
		}
	}
	else {
		generator.addRegImm32(RSI, offset * 4);
		if (instr.l()) {
			generator.lea64(RDX, RDI, GPR(reg));
			generator.jumpAbs(RAX, (uint64_t)loadMemory<uint32_t>);
		}
		else {
			generator.loadMem32(RDX, RDI, GPR(reg));
			generator.jumpAbs(RAX, (uint64_t)storeMemory<uint32_t>);
		}
	}
}

void ARMThumbGenerator::generateLoadStoreMultiple(ARMThumbInstr instr) {
	int reg = (instr.value >> 8) & 7;
	
	generator.pushReg64(RBX);
	generator.pushReg64(RBP);
	generator.pushReg64(RBP);
	
	generator.movReg64(RBP, RDI);
	generator.loadMem32(RBX, RDI, GPR(reg));
	
	for (int i = 0; i < 8; i++) {
		if (instr.value & (1 << i)) {
			generator.movReg64(RDI, RBP);
			generator.movReg32(RSI, RBX);
			if (instr.l()) {
				generator.lea64(RDX, RDI, GPR(i));
				generator.callAbs(RAX, (uint64_t)loadMemory<uint32_t>);
				generateResultCheck();
			}
			else {
				generator.loadMem32(RDX, RDI, GPR(i));
				generator.callAbs(RAX, (uint64_t)storeMemory<uint32_t>);
				generateResultCheck();
			}
			generator.addRegImm32(RBX, 4);
		}
	}
	
	generator.storeMem32(RBP, GPR(reg), RBX);
	
	generator.popReg64(RBP);
	generator.popReg64(RBP);
	generator.popReg64(RBX);
	generator.ret();
}

void ARMThumbGenerator::generatePushPopRegisterList(ARMThumbInstr instr) {
	generator.pushReg64(RBX);
	generator.pushReg64(RBP);
	generator.pushReg64(RBP);
	
	generator.movReg64(RBP, RDI);
	generator.loadMem32(RBX, RDI, REG(SP));
	
	if (instr.l()) {
		for (int i = 0; i < 8; i++) {
			if (instr.value & (1 << i)) {
				generator.movReg64(RDI, RBP);
				generator.movReg32(RSI, RBX);
				generator.lea64(RDX, RDI, GPR(i));
				generator.callAbs(RAX, (uint64_t)loadMemory<uint32_t>);
				generateResultCheck();
				generator.addRegImm32(RBX, 4);
			}
		}
		if (instr.r()) {
			generator.movReg64(RDI, RBP);
			generator.movReg32(RSI, RBX);
			generator.lea64(RDX, RDI, REG(PC));
			generator.callAbs(RAX, (uint64_t)loadMemory<uint32_t>);
			generateResultCheck();
			generator.bitTestResetMem32(RBP, REG(PC), 0);
			generator.jumpIfCarry(8);
			generator.bitTestResetMem32(RBP, CPSR, 5); // 8 bytes
			generator.addRegImm32(RBX, 4);
		}
	}
	else {
		if (instr.r()) {
			generator.subRegImm32(RBX, 4);
			generator.movReg32(RSI, RBX);
			generator.loadMem32(RDX, RDI, REG(LR));
			generator.callAbs(RAX, (uint64_t)storeMemory<uint32_t>);
			generateResultCheck();
		}
		for (int i = 7; i >= 0; i--) {
			if (instr.value & (1 << i)) {
				generator.subRegImm32(RBX, 4);
				generator.movReg64(RDI, RBP);
				generator.movReg32(RSI, RBX);
				generator.loadMem32(RDX, RDI, GPR(i));
				generator.callAbs(RAX, (uint64_t)storeMemory<uint32_t>);
				generateResultCheck();
			}
		}
	}
	
	generator.storeMem32(RBP, REG(SP), RBX);
	
	generator.popReg64(RBP);
	generator.popReg64(RBP);
	generator.popReg64(RBX);
	generator.ret();
}

void ARMThumbGenerator::generateReadReg(Register target, int reg) {
	generator.loadMem32(target, RDI, GPR(reg));
	if (reg == ARMCore::PC) {
		generator.addRegImm32(target, 2);
	}
}

void ARMThumbGenerator::generateAddReg(Register reg, Register other) {
	generator.addRegReg32(reg, other);
	generateOverflowUpdate();
}

void ARMThumbGenerator::generateSubReg(Register reg, Register other) {
	generator.subRegReg32(reg, other);
	generateOverflowUpdateInv();
}

void ARMThumbGenerator::generateAddRegCarry(Register reg, Register other) {
	generator.bitTestMem32(RDI, CPSR, 29);
	generator.adcRegReg32(reg, other);
	generateOverflowUpdate();
}

void ARMThumbGenerator::generateSubRegCarry(Register reg, Register other) {
	generator.bitTestMem32(RDI, CPSR, 29);
	generator.flipCarry();
	generator.sbbRegReg32(reg, other);
	generateOverflowUpdateInv();
}

void ARMThumbGenerator::generateFlagsUpdate(Register reg) {
	generator.andMemImm32(RDI, CPSR, 0x3FFFFFFF); // 10 bytes
	generator.testReg32(reg, reg); // 2 bytes
	generator.jumpIfNotZero(8); // 2 bytes
	generator.bitTestSetMem32(RDI, CPSR, 30); // 8 bytes
	generator.bitTestReg32(reg, 31); // 4 bytes
	generator.jumpIfNotCarry(8); // 2 bytes
	generator.bitTestSetMem32(RDI, CPSR, 31); // 8 bytes
}

void ARMThumbGenerator::generateOverflowUpdate() {
	generator.jumpIfCarry(32); // 2 bytes
	generator.jumpIfOverflow(12); // 2 bytes
	
	// 12 bytes (~C & ~V)
	generator.andMemImm32(RDI, CPSR, 0xCFFFFFFF);
	generator.jumpRel(48);
	
	// 18 bytes (~C & V)
	generator.bitTestResetMem32(RDI, CPSR, 29);
	generator.bitTestSetMem32(RDI, CPSR, 28);
	generator.jumpRel(30);
	
	generator.jumpIfOverflow(18); // 2 bytes
	
	// 18 bytes (C & ~V)
	generator.bitTestSetMem32(RDI, CPSR, 29);
	generator.bitTestResetMem32(RDI, CPSR, 28);
	generator.jumpRel(10);
	
	// 10 bytes (C & V)
	generator.orMemImm32(RDI, CPSR, 0x30000000);
}

void ARMThumbGenerator::generateOverflowUpdateInv() {
	generator.jumpIfNotCarry(32); // 2 bytes
	generator.jumpIfOverflow(12); // 2 bytes
	
	// 12 bytes (~C & ~V)
	generator.andMemImm32(RDI, CPSR, 0xCFFFFFFF);
	generator.jumpRel(48);
	
	// 18 bytes (~C & V)
	generator.bitTestResetMem32(RDI, CPSR, 29);
	generator.bitTestSetMem32(RDI, CPSR, 28);
	generator.jumpRel(30);
	
	generator.jumpIfOverflow(18); // 2 bytes
	
	// 18 bytes (C & ~V)
	generator.bitTestSetMem32(RDI, CPSR, 29);
	generator.bitTestResetMem32(RDI, CPSR, 28);
	generator.jumpRel(10);
	
	// 10 bytes (C & V)
	generator.orMemImm32(RDI, CPSR, 0x30000000);
}

void ARMThumbGenerator::generateResultCheck() {
	generator.testReg32(RAX, RAX);
	generator.jumpIfNotZero(4);
	generator.popReg64(RBP);
	generator.popReg64(RBP);
	generator.popReg64(RBX);
	generator.ret();
}
