
#pragma once

#include "cpu/arm/armprocessor.h"
#include "debugger/interface.h"
#include "debugger/expression.h"
#include "config.h"
#include <string>
#include <cstdint>

class ARMDebugger : public DebugInterface {
public:
	ARMDebugger(PhysicalMemory *physmem, ARMProcessor *cpu);
	
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
	void printThreads();
	void printThreadDetails(uint32_t id);
	void printMessageQueues();
	void printMessageQueueDetails(uint32_t id);
	void printDevices();
	void printVolumes();
	void printFileClients();
	void printSlcCacheState();
	
	#if STATS
	void printStats();
	#endif
	
private:
	std::string getQueueSuffix(uint32_t id);
	
	PhysicalMemory *physmem;
	ARMProcessor *cpu;
};