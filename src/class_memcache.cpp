
#include "class_memcache.h"

#include <cstring>

MemoryCache::MemoryCache() {
	invalidate();
}

void MemoryCache::invalidate() {
	memset(lines, 0, sizeof(lines));
}

void *MemoryCache::alloc(uint32_t addr) {
	int line = (addr >> CacheBits) & LineMask;
	return lines[line].data;
}

void MemoryCache::enable(uint32_t addr) {
	int line = (addr >> CacheBits) & LineMask;
	lines[line].valid = true;
	lines[line].addr = addr & ~CacheMask;
}
