
#include "interface_virtmem.h"

class MMUCache {
	public:
	MMUCache();
	
	void invalidate();
	void update(IVirtualMemory::Access type, uint32_t virtAddr, uint32_t physAddr, uint32_t mask);
	bool translate(uint32_t *addr, uint32_t length, IVirtualMemory::Access type);
	
	private:
	//Remeber to update these if IVirtualMemory::Access is changed
	bool isValid[3];
	uint32_t cachedStart[3];
	uint32_t cachedEnd[3];
	uint32_t cachedPhys[3];
	uint32_t addrMask[3];
};
