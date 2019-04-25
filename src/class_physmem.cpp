
#include "class_physmem.h"
#include "class_range.h"
#include "errors.h"
#include <cstdint>

SpecialRange::SpecialRange(uint32_t start, uint32_t end, ReadCB readCB, WriteCB writeCB)
	: Range(start, end), readCB(readCB), writeCB(writeCB) {}
	
PhysicalRange::PhysicalRange(uint32_t start, uint32_t end, char *buffer)
	: Range(start, end), buffer(buffer) {}
	
PhysicalMemory::PhysicalMemory() : prevRange(nullptr) {}
	
PhysicalMemory::~PhysicalMemory() {
	for (PhysicalRange &r : ranges) {
		delete[] r.buffer;
	}
}

bool PhysicalMemory::checkOverlap(uint32_t start, uint32_t end) {
	for (Range &r : ranges) {
		if (r.collides(start, end)) {
			ValueError("Memory range (0x%08x, 0x%08x) overlaps existing range (0x%08x, 0x%08x)",
				start, end, r.start, r.end);
			return false;
		}
	}
	
	for (Range &r : specialRanges) {
		if (r.collides(start, end)) {
			ValueError("Memory range (0x%08x, 0x%08x) overlaps special range (0x%08x, 0x%08x)",
				start, end, r.start, r.end);
			return false;
		}
	}
	return true;
}

bool PhysicalMemory::checkSize(uint32_t start, uint32_t end) {
	if (end - start >= 0x7FFFFFFF) {
		OverflowError("Memory range is too big");
		return false;
	}
	return true;
}

bool PhysicalMemory::addRange(uint32_t start, uint32_t end) {
	if (!checkOverlap(start, end)) return false;
	if (!checkSize(start, end)) return false;
	
	PhysicalRange range(start, end, new char[end - start + 1]);
	ranges.push_back(range);
	
	prevRange = nullptr;
	return true;
}

bool PhysicalMemory::addSpecial(uint32_t start, uint32_t end, ReadCB readCB, WriteCB writeCB) {
	if (!checkOverlap(start, end)) return false;
	if (!checkSize(start, end)) return false;
	
	SpecialRange range(start, end, readCB, writeCB);
	
	specialRanges.push_back(range);
	return true;
}

int PhysicalMemory::read(uint32_t addr, void *data, uint32_t length) {
	uint32_t end = addr + length - 1;

	if (end >= addr) {
		if (prevRange && prevRange->contains(addr, end)) {
			memcpy(data, prevRange->buffer + addr - prevRange->start, length);
			return 0;
		}
		
		for (PhysicalRange &r : ranges) {
			if (r.contains(addr, end)) {
				memcpy(data, r.buffer + addr - r.start, length);
				prevRange = &r;
				return 0;
			}
		}
		
		for (SpecialRange &r : specialRanges) {
			if (r.contains(addr, end)) {
				return r.readCB(addr, data, length) ? 0 : -1;
			}
		}
	}
	
	ValueError("Illegal memory read: addr=0x%08x length=0x%x", addr, length);
	return -2;
}

int PhysicalMemory::write(uint32_t addr, const void *data, uint32_t length) {
	uint32_t end = addr + length - 1;
	
	if (end >= addr) {
		if (prevRange && prevRange->contains(addr, end)) {
			memcpy(prevRange->buffer + addr - prevRange->start, data, length);
			return 0;
		}
		
		for (PhysicalRange &r : ranges) {
			if (r.contains(addr, end)) {
				memcpy(r.buffer + addr - r.start, data, length);
				prevRange = &r;
				return 0;
			}
		}
		
		for (SpecialRange &r : specialRanges) {
			if (r.contains(addr, end)) {
				return r.writeCB(addr, data, length) ? 0 : -1;
			}
		}
	}
	
	ValueError("Illegal memory write: addr=0x%08x length=0x%x", addr, length);
	return -2;
}
