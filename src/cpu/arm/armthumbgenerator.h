
#pragma once

#include "cpu/arm/armthumbinstr.h"
#include "cpu/x86codegenerator.h"
#include "physicalmemory.h"
#include "config.h"

#include <cstdint>


class ARMProcessor;


class ARMThumbGenerator {
public:
	ARMThumbGenerator();
	
	char *get();
	size_t size();
	
	void generate(uint16_t value);
	
private:
	void generateInstr(ARMThumbInstr instr);
	void generateError(ARMThumbInstr instr);
	void generateUndefined();
	void generateSoftwareInterrupt();
	void generateMoveShifted(ARMThumbInstr instr);
	void generateAddSubtract(ARMThumbInstr instr);
	void generateAddSubCmpMovImm(ARMThumbInstr instr);
	void generateSpecialDataProcessing(ARMThumbInstr instr);
	void generateDataProcessing(ARMThumbInstr instr);
	void generateUnconditionalBranch(ARMThumbInstr instr);
	void generateConditionalBranch(ARMThumbInstr instr);
	void generateLongBranchWithLink(ARMThumbInstr instr);
	void generateBranchExchange(ARMThumbInstr instr);
	void generateAdjustStackPointer(ARMThumbInstr instr);
	void generateAddToSPOrPC(ARMThumbInstr instr);
	void generateLoadPCRelative(ARMThumbInstr instr);
	void generateLoadStoreStack(ARMThumbInstr instr);
	void generateLoadStoreMultiple(ARMThumbInstr instr);
	void generateLoadStoreHalf(ARMThumbInstr instr);
	void generateLoadStoreRegOffs(ARMThumbInstr instr);
	void generateLoadStoreImmOffs(ARMThumbInstr instr);
	void generatePushPopRegisterList(ARMThumbInstr instr);
	
	void generateReadReg(Register target, int reg);
	void generateAddReg(Register reg, Register other);
	void generateSubReg(Register reg, Register other);
	void generateAddRegCarry(Register reg, Register other);
	void generateSubRegCarry(Register reg, Register other);
	void generateFlagsUpdate(Register reg);
	void generateOverflowUpdate();
	void generateOverflowUpdateInv();
	void generateResultCheck();

	X86CodeGenerator generator;
	uint32_t offsets[0x800];
	int index;
};
