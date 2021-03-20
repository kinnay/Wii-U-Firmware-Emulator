
#pragma once

#include "cpu/arm/arminstruction.h"
#include "cpu/x86codegenerator.h"
#include "physicalmemory.h"
#include "config.h"

#include <cstdint>


class ARMProcessor;


class ARMCodeGenerator {
public:
	ARMCodeGenerator();
	
	char *get();
	size_t size();
	
	void generate(uint32_t value);
	
private:
	void generateReadReg(Register target, int reg);
	void generateReadShifted(Register target, ARMInstruction instr, bool s);
	void generateReadShiftedReg(Register target, ARMInstruction instr, bool s);
	void generateReadShiftedImm(Register target, ARMInstruction instr, bool s);
	
	void generateFlagsUpdate(Register reg);
	void generateCarryUpdate(); // 14 bytes
	void generateOverflowUpdate();
	void generateOverflowUpdateInv();
	
	void generateAddImm(Register reg, uint32_t imm, bool s);
	void generateAddImmCarry(Register reg, uint32_t imm, bool s);
	void generateAddReg(Register reg, Register other, bool s);
	void generateAddRegCarry(Register reg, Register other, bool s);
	
	void generateSubImm(Register reg, uint32_t imm, bool s);
	void generateSubImmCarry(Register reg, uint32_t imm, bool s);
	void generateSubReg(Register reg, Register other, bool s);
	void generateSubRegCarry(Register reg, Register other, bool s);
	
	void generateInstr(ARMInstruction instr);
	void generateCondition(ARMInstruction instr); // up to 23 bytes
	void generateUnimplemented(ARMInstruction instr);
	void generateError(ARMInstruction instr);
	void generateUndefined();
	void generateSoftwareInterrupt();
	void generateBranch(ARMInstruction instr);
	void generateBranchExchange(ARMInstruction instr);
	void generateBranchLinkExchange(ARMInstruction instr);
	void generateReadStatusReg(ARMInstruction instr);
	void generateDataProcessingImm(ARMInstruction instr);
	void generateDataProcessingReg(ARMInstruction instr);
	void generateLoadStoreImm(ARMInstruction instr);
	void generateLoadStoreReg(ARMInstruction instr);
	void generateLoadStoreMultiple(ARMInstruction instr);
	
	template <class T>
	void generateLoadStore(ARMInstruction instr, bool exchange);
	
	X86CodeGenerator generator;
	uint32_t offsets[0x400];
	int index;
};
