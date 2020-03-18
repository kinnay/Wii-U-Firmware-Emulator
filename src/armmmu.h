
#pragma once

#include "physicalmemory.h"
#include "armcore.h"
#include "mmucache.h"
#include "enum.h"

#include <cstdint>


class ARMMMU {
public:
	ARMMMU(PhysicalMemory *physmem, ARMCore *core);
	
	void reset();
	
	bool translate(uint32_t *addr, MemoryAccess type, bool supervisor);
	
	MMUCache cache;

private:
	bool translateFromTable(uint32_t *addr, MemoryAccess type, bool supervisor);
	bool checkPermissions(MemoryAccess type, bool supervisor, int ap);
	void signalFault(MemoryAccess type, uint32_t addr, int domain, int status);
	
	PhysicalMemory *physmem;
	ARMCore *core;
};
