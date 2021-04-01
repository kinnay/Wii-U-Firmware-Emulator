
#pragma once

#include "cpu/processor.h"
#include "debugger/expression.h"
#include "common/refcountedobj.h"
#include <string>
#include <cstdint>

class DebugInterface : public RefCountedObj {
public:
	virtual uint32_t pc() = 0;
	virtual std::string name() = 0;
	virtual Ref<EvalContext> getContext() = 0;
	virtual Processor *getProcessor() = 0;
	virtual bool translate(uint32_t *address) = 0;
	virtual std::string format() = 0;
	
	virtual void printState() = 0;
	virtual void printStateDetailed() = 0;
	virtual void printStackTrace() = 0;
	virtual void printMemoryMap() = 0;
	virtual void printThreads() = 0;
	virtual void printThreadDetails(uint32_t id) = 0;
};
