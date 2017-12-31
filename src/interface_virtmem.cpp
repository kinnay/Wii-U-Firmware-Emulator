
#include "interface_virtmem.h"

IVirtualMemory::IVirtualMemory() : cacheEnabled(false) {}

void IVirtualMemory::setCacheEnabled(bool enabled) {
	cacheEnabled = enabled;
}

void IVirtualMemory::invalidateCache() {
	cache.invalidate();
}
