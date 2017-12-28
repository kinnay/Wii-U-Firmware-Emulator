
#pragma once

#include "interface_virtmem.h"
#include "interface_physmem.h"

class PPCMMU : public IVirtualMemory {
	public:
	PPCMMU(IPhysicalMemory *phys, bool bigEndian);
	bool translate(uint32_t *addr, uint32_t length, Access type);
	
	void setDataTranslation(bool enabled);
	void setInstructionTranslation(bool enabled);
	void setSupervisorMode(bool enabled);
	
	void setRpnSize(int bits);
	
	uint32_t dbatu[8];
	uint32_t dbatl[8];
	uint32_t ibatu[8];
	uint32_t ibatl[8];
	
	uint32_t sdr1;
	uint32_t sr[16];
	
	private:
	bool read32(uint32_t addr, uint32_t *value);
	bool translateBAT(uint32_t *addr, uint32_t *batu, uint32_t *batl, bool write);
	bool searchPageTable(
		uint32_t *addr, uint32_t vsid, uint32_t pageIndex,
		uint32_t hash, bool secondary, int key, bool write
	);
	
	IPhysicalMemory *physmem;
	
	bool swapEndian;
	bool dataTranslation;
	bool instrTranslation;
	bool supervisorMode;
	
	int pageIndexShift;
	uint32_t pageIndexMask;
	uint32_t byteOffsetMask;
	int apiShift;
};
