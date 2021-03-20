
#pragma once

#include "cpu/ppc/ppcprocessor.h"
#include "debugger/interface.h"
#include "debugger/expression.h"
#include "common/refcountedobj.h"
#include <string>
#include <cstdint>

struct PPCModule : public RefCountedObj {
	static bool compareText(Ref<PPCModule> a, Ref<PPCModule> b);
	
	std::string path;
	std::string name;
	uint64_t text;
	uint64_t textsize;
	uint64_t data;
	uint64_t datasize;
};

class PPCDebugger : public DebugInterface {
public:
	typedef std::vector<Ref<PPCModule>> ModuleList;

	PPCDebugger(PhysicalMemory *physmem, PPCProcessor *cpu, int index);
	
	uint32_t pc();
	std::string name();
	Ref<EvalContext> getContext();
	Processor *getProcessor();
	bool translate(uint32_t *address);
	
	void step();
	
	void printState();
	void printStateDetailed();
	void printStackTrace();
	void printMemoryMap();
	void printModules();
	void printModuleDetails(std::string name);
	void printThreads();
	void printThreadDetails(uint32_t thread);
	void printIPC();
	
	#if STATS
	void printStats();
	#endif
	
private:
	void printBAT(uint32_t batu, uint32_t batl);
	void printIPCRequest(uint32_t context);
	void printStackTraceForThread(uint32_t sp, ModuleList &modules);
	void printThreadWait(uint32_t thread);
	
	std::string formatAddress(uint32_t addr, ModuleList &modules);
	ModuleList getModules();
	
	PhysicalMemory *physmem;
	PPCProcessor *cpu;
	int index;
};
