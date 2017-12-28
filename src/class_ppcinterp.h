
#pragma once

#include "class_interpreter.h"
#include "class_ppccore.h"
#include "class_endian.h"
#include "interface_virtmem.h"
#include "interface_physmem.h"
#include <functional>
#include <vector>

class PPCInterpreter : public Interpreter {
	public:
	
	PPCInterpreter(PPCCore *core, IPhysicalMemory *physmem, IVirtualMemory *virtmem, bool bigEndian);
	bool step();
	
	uint32_t getPC() { return core->pc; }
	
	PPCCore *core;
	
	private:
	IPhysicalMemory *physmem;
	bool swapEndian;
};
