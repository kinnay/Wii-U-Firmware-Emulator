
#pragma once

#include "enum.h"
#include "config.h"

#include <cstdint>


struct MMUCacheEntry {
	uint32_t vaddr;
	uint32_t paddr;
	uint32_t mask;
};


class MMUCache {
public:
	MMUCache();
	
	void reset();
	void invalidate();
	void update(MemoryAccess type, bool supervisor, uint32_t vaddr, uint32_t paddr, uint32_t mask);
	
	inline bool translate(uint32_t *addr, MemoryAccess type, bool supervisor) {
		int index = (int)type * 2 + supervisor;
		uint32_t mask = entries[index].mask;
		if ((*addr & ~mask) == entries[index].vaddr) {
			*addr = (*addr & mask) | entries[index].paddr;
			#if STATS
			hits++;
			#endif
			return true;
		}
		#if STATS
		misses++;
		#endif
		return false;
	}
	
	#if STATS
	uint64_t hits;
	uint64_t misses;
	#endif
	
private:
	MMUCacheEntry entries[6];
};
