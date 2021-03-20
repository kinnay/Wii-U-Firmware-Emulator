
#pragma once

#include "cpu/ppc/ppccore.h"
#include "cpu/mmucache.h"
#include "physicalmemory.h"
#include "enum.h"

#include <cstdint>


class PPCMMU {
public:
	PPCMMU(PhysicalMemory *physmem, PPCCore *core);
	
	void reset();
	bool translate(uint32_t *addr, MemoryAccess type, bool supervisor);
	
	MMUCache cache;
	
private:
	void translatePhysical(uint32_t *addr);
	bool translateVirtual(uint32_t *addr, MemoryAccess type, bool supervisor);
	bool translateBAT(uint32_t *addr, int bat, MemoryAccess type, bool supervisor);
	bool searchPageTable(
		uint32_t *addr, uint32_t vsid, uint32_t pageIndex, uint32_t hash,
		bool secondary, int key, MemoryAccess type, bool supervisor
	);
	
	PhysicalMemory *physmem;
	PPCCore *core;
};
