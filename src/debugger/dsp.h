
#pragma once

#include "cpu/dsp.h"

#include "debugger/interface.h"
#include "debugger/expression.h"

#include "config.h"

#include <utility>


class DSPDebugger : public DebugInterface {
public:
	DSPDebugger(DSPInterpreter *cpu);
	
	uint32_t pc();
	std::string name();
	Ref<EvalContext> getContext();
	Processor *getProcessor();
	bool translate(uint32_t *address);
	std::string format();
	
	void printState();
	void printStateDetailed();
	void printStackTrace();
	void printMemoryMap();
	void printThreads();
	void printThreadDetails(uint32_t id);
	
	Buffer read(uint32_t address, uint32_t length, bool code);
	
	#if STATS
	void printStats();
	#endif
	
private:
	void printStack(int index);
	
	std::pair<void *, uint32_t> getPtr(uint32_t address, bool code);
	
	DSPInterpreter *cpu;
};