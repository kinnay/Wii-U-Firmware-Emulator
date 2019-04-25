
#include "class_mmucache.h"
#include <cstring>

MMUCache::MMUCache() {
	invalidate();
}

void MMUCache::invalidate() {
	memset(&entries, 0, sizeof(entries));
}

void MMUCache::update(int type, uint32_t virtAddr, uint32_t physAddr, uint32_t mask) {
	entries[type].virtAddr = virtAddr & ~mask;
	entries[type].physAddr = physAddr;
	entries[type].mask = mask;
	entries[type].valid = true;
}

bool MMUCache::translate(uint32_t *addr, int type) {
	if (entries[type].valid) {
		uint32_t mask = entries[type].mask;
		if ((*addr & ~mask) == entries[type].virtAddr) {
			*addr = (*addr & mask) | entries[type].physAddr;
			return true;
		}
	}
	return false;
}
