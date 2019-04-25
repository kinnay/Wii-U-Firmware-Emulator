
#pragma once

#include "class_interpreter.h"
#include "class_ppccore.h"
#include "interface_virtmem.h"
#include "class_physmem.h"

class PPCInterpreter : public Interpreter {
public:	
	PPCInterpreter(PPCCore *core, PhysicalMemory *physmem, IVirtualMemory *virtmem);
	bool step();
	
	uint32_t getPC() { return core->pc; }
	
	PPCCore *core;
	
private:
	PhysicalMemory *physmem;
	
	bool handleMsrWrite(uint32_t value);
};
