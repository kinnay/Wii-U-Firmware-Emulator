
#include "class_virtmem.h"
#include "range.h"
#include "errors.h"
#include <cstdint>

MemoryRange::MemoryRange(uint32_t virt, uint32_t phys, uint32_t length)
	: Range(virt, length), phys(phys) {}
	
VirtualMemory::~VirtualMemory() {
	for (MemoryRange *r : ranges) {
		delete r;
	}
}

bool VirtualMemory::addRange(uint32_t virt, uint32_t phys, uint32_t length) {
	for (Range *r : ranges) {
		if (r->collides(virt, length)) {
			ValueError("Memory range (0x%08X, 0x%08X) overlaps existing range (0x%08x, 0x%08x)",
				virt, length, r->start, r->length);
			return false;
		}
	}
	
	MemoryRange *range = new MemoryRange(virt, phys, length);
	if (!range) {
		MemoryError("Couldn't allocate range object");
		return false;
	}
	
	ranges.push_back(range);
	return true;
}

bool VirtualMemory::translate(uint32_t *addr, uint32_t length, Access type) {
	for (MemoryRange *r : ranges) {
		if (r->contains(*addr, length)) {
			*addr = r->phys + (*addr) - r->start;
			return true;
		}
	}
	
	ValueError("Illegal memory access: addr=0x%08x length=0x%x", *addr, length);
	return false;
}
