
#pragma once

#include "interface_virtmem.h"
#include "interface_physmem.h"

class PPCMMU : public IVirtualMemory {
	public:
	PPCMMU(IPhysicalMemory *phys, bool bigEndian);
	bool translate(uint32_t *addr, uint32_t length, Access type);
	
	void setRpnSize(int bits);
	
	uint32_t dbatu[8];
	uint32_t dbatl[8];
	uint32_t ibatu[8];
	uint32_t ibatl[8];
	
	uint32_t sdr1;
	uint32_t sr[16];
	
	private:
	bool read32(uint32_t addr, uint32_t *value);
	bool translateBAT(uint32_t *addr, uint32_t *batu, uint32_t *batl, Access type);
	bool searchPageTable(
		uint32_t *addr, uint32_t vsid, uint32_t pageIndex,
		uint32_t hash, bool secondary, int key, Access type
	);
	
	IPhysicalMemory *physmem;
	
	bool swapEndian;
	
	int pageIndexShift;
	uint32_t pageIndexMask;
	uint32_t byteOffsetMask;
	int apiShift;
};
