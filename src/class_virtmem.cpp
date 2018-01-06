
#include "class_virtmem.h"
#include "class_range.h"
#include "errors.h"
#include <cstdint>

MemoryRange::MemoryRange(uint32_t start, uint32_t end, uint32_t phys)
	: Range(start, end), phys(phys) {}
	
VirtualMemory::~VirtualMemory() {
	for (MemoryRange *r : ranges) {
		delete r;
	}
}

bool VirtualMemory::addRange(uint32_t start, uint32_t end, uint32_t phys) {
	for (Range *r : ranges) {
		if (r->collides(start, end)) {
			ValueError("Memory range (0x%08X, 0x%08X) overlaps existing range (0x%08x, 0x%08x)",
				start, end, r->start, r->end);
			return false;
		}
	}
	
	MemoryRange *range = new MemoryRange(start, end, phys);
	if (!range) {
		MemoryError("Couldn't allocate range object");
		return false;
	}
	
	ranges.push_back(range);
	return true;
}

bool VirtualMemory::translate(uint32_t *addr, uint32_t length, Access type) {
	uint32_t start = *addr;
	uint32_t end = start + length - 1;
	for (MemoryRange *r : ranges) {
		if (r->contains(start, end)) {
			*addr = r->phys + start - r->start;
			return true;
		}
	}
	
	ValueError("Illegal memory access: addr=0x%08x length=0x%x", start, length);
	return false;
}
