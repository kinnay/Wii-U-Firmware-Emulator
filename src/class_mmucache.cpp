
#include "class_mmucache.h"
#include <cstring>

MMUCache::MMUCache() {
	invalidate();
}

void MMUCache::invalidate() {
	memset(isValid, 0, sizeof(isValid));
}

void MMUCache::update(int type, uint32_t virtAddr, uint32_t physAddr, uint32_t mask) {
	virtAddr &= ~mask;
	cachedStart[type] = virtAddr;
	cachedEnd[type] = virtAddr + mask;
	cachedPhys[type] = physAddr;
	addrMask[type] = mask;
	isValid[type] = true;
}

bool MMUCache::translate(uint32_t *addr, uint32_t length, int type) {
	if (isValid[type]) {
		if (cachedStart[type] <= *addr && *addr <= cachedEnd[type]) {
			*addr = (*addr & addrMask[type]) | cachedPhys[type];
			return true;
		}
	}
	return false;
}
