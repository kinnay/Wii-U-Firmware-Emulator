
#pragma once

#include "physicalmemory.h"
#include "armthumbinstr.h"
#include "x86codegenerator.h"
#include "config.h"

#include <cstdint>


class ARMProcessor;


class ThumbCodeGenerator {
public:
	ThumbCodeGenerator();
	
	char *get();
	size_t size();
	
	void generate(ARMThumbInstr instr);
	
private:
	void generateInstr(ARMThumbInstr instr);
	void generateError(ARMThumbInstr instr);
	void generateUnimplemented(ARMThumbInstr instr);
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


class ARMThumbJIT {
public:
	typedef bool (*JITEntryFunc)(ARMProcessor *cpu);
	
	ARMThumbJIT(PhysicalMemory *physmem, ARMProcessor *cpu);
	
	void reset();
	void execute(uint32_t pc);
	void invalidate();
	
	#if STATS
	uint64_t instrsExecuted;
	uint64_t instrsCompiled;
	uint64_t instrSize;
	#endif
	
private:
	char *generateCode(uint32_t pc);
	
	PhysicalMemory *physmem;
	ARMProcessor *cpu;
	
	char *table[0x100000];
	uint32_t sizes[0x100000];
};
