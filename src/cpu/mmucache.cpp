
#include "mmucache.h"

#include "common/logger.h"

#include <cstring>


MMUCache::MMUCache() {
	reset();
}

void MMUCache::reset() {
	invalidate();
	
	#if STATS
	hits = 0;
	misses = 0;
	#endif
}

void MMUCache::invalidate() {
	memset(entries, 0xFF, sizeof(entries));
}

void MMUCache::update(MemoryAccess type, bool supervisor, uint32_t vaddr, uint32_t paddr, uint32_t mask) {
	int index = (int)type * 2 + supervisor;
	
	entries[index].vaddr = vaddr & ~mask;
	entries[index].paddr = paddr;
	entries[index].mask = mask;
}

bool MMUCache::translate(uint32_t *addr, MemoryAccess type, bool supervisor) {
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
