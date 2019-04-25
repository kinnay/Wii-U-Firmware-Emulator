
#pragma once

#include <cstdint>

class MMUCacheEntry {
public:
	bool valid;
	uint32_t virtAddr;
	uint32_t physAddr;
	uint32_t mask;
};


class MMUCache {
public:
	MMUCache();
	
	void invalidate();
	void update(int type, uint32_t virtAddr, uint32_t physAddr, uint32_t mask);
	bool translate(uint32_t *addr, int type);
	
private:
	MMUCacheEntry entries[3];
};
