
#pragma once

#include "physicalmemory.h"
#include "ppcinstruction.h"
#include "x86codegenerator.h"
#include "config.h"

#include <cstdint>


class PPCProcessor;


class PPCCodeGenerator {
public:
	PPCCodeGenerator();
	
	char *get();
	size_t size();
	
	void generate(PPCInstruction instr);
	
private:
	void generateInstr(PPCInstruction instr);
	void generateError(PPCInstruction instr);
	void generateUnimplemented(PPCInstruction instr);
	
	void generateConditionCheck(PPCInstruction instr);
	void generateBc(PPCInstruction instr);
	void generateB(PPCInstruction instr);
	
	void generateAddi(PPCInstruction instr);
	void generateAddis(PPCInstruction instr);
	void generateRlwimi(PPCInstruction instr);
	void generateRlwinm(PPCInstruction instr);
	void generateOri(PPCInstruction instr);
	void generateOris(PPCInstruction instr);
	void generateXori(PPCInstruction instr);
	void generateXoris(PPCInstruction instr);
	void generateAndi(PPCInstruction instr);
	void generateAndis(PPCInstruction instr);
	
	void generateAdd(PPCInstruction instr);
	
	void generateLwbrx(PPCInstruction instr);
	void generateStwbrx(PPCInstruction instr);
	
	void generateLfsx(PPCInstruction instr);
	void generateLfdx(PPCInstruction instr);
	
	void generateStfsx(PPCInstruction instr);
	void generateStfdx(PPCInstruction instr);
	
	void generateStfiwx(PPCInstruction instr);
	
	template <class T> void generateLoad(PPCInstruction instr);
	template <class T> void generateLoadu(PPCInstruction instr);
	
	void generateFlagsUpdate(bool rc);
	
	#if METRICS
	void generateMetricsUpdate(PPCInstruction instr);
	#endif

	X86CodeGenerator generator;
	uint32_t offsets[0x400];
	int index;
};


class PPCJIT {
public:
	typedef bool (*JITEntryFunc)(PPCProcessor *cpu);
	
	PPCJIT(PhysicalMemory *physmem, PPCProcessor *cpu);
	
	void reset();
	void execute(uint32_t pc);
	void invalidate();
	void invalidateBlock(uint32_t addr);
	
	#if STATS
	uint64_t instrsExecuted;
	uint64_t instrsCompiled;
	uint64_t instrSize;
	uint64_t blocksInvalidated;
	#endif
	
private:
	char *generateCode(uint32_t pc);
	
	PhysicalMemory *physmem;
	PPCProcessor *cpu;
	
	char *table[0x100000];
	uint32_t sizes[0x100000];
};
