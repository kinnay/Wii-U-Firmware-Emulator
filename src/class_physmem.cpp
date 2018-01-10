
#include "class_physmem.h"
#include "class_range.h"
#include "errors.h"
#include <cstdint>

SpecialRange::SpecialRange(uint32_t start, uint32_t end, ReadCB readCB, WriteCB writeCB)
	: Range(start, end), readCB(readCB), writeCB(writeCB) {}
	
PhysicalRange::PhysicalRange(uint32_t start, uint32_t end) : Range(start, end), buffer(0) {}
	
PhysicalMemory::PhysicalMemory() : prevRange(0) {}
	
PhysicalMemory::~PhysicalMemory() {
	for (PhysicalRange *r : ranges) {
		if (r->buffer) delete r->buffer;
		delete r;
	}
	
	for (SpecialRange *r : specialRanges) {
		delete r;
	}
}

bool PhysicalMemory::checkOverlap(uint32_t start, uint32_t end) {
	for (Range *r : ranges) {
		if (r->collides(start, end)) {
			ValueError("Memory range (0x%08x, 0x%08x) overlaps existing range (0x%08x, 0x%08x)",
				start, end, r->start, r->end);
			return false;
		}
	}
	
	for (Range *r : specialRanges) {
		if (r->collides(start, end)) {
			ValueError("Memory range (0x%08x, 0x%08x) overlaps special range (0x%08x, 0x%08x)",
				start, end, r->start, r->end);
			return false;
		}
	}
	return true;
}

bool PhysicalMemory::addRange(uint32_t start, uint32_t end) {
	if (!checkOverlap(start, end)) return false;
	
	PhysicalRange *range = new PhysicalRange(start, end);
	if (!range) {
		MemoryError("Couldn't allocate range object");
		return false;
	}
	
	void *buffer = new char[end - start + 1];
	if (!buffer) {
		MemoryError("Couldn't allocate memory buffer");
		delete range;
		return false;
	}
	range->buffer = buffer;
	
	ranges.push_back(range);
	prevRange = 0;
	return true;
}

bool PhysicalMemory::addSpecial(uint32_t start, uint32_t end, ReadCB readCB, WriteCB writeCB) {
	if (!checkOverlap(start, end)) return false;
	
	SpecialRange *range = new SpecialRange(start, end, readCB, writeCB);
	
	if (!range) {
		MemoryError("Couldn't allocate range object");
		return false;
	}
	
	specialRanges.push_back(range);
	return true;
}

int PhysicalMemory::read(uint32_t addr, void *data, uint32_t length) {
	uint32_t end = addr + length - 1;
	
	if (prevRange && prevRange->contains(addr, end)) {
		memcpy(data, (char *)prevRange->buffer + addr - prevRange->start, length);
		return 0;
	}
	
	for (PhysicalRange *r : ranges) {
		if (r->contains(addr, end)) {
			memcpy(data, (char *)r->buffer + addr - r->start, length);
			prevRange = r;
			return 0;
		}
	}
	
	for (SpecialRange *r : specialRanges) {
		if (r->contains(addr, end)) {
			return r->readCB(addr, data, length) ? 0 : -1;
		}
	}
	
	ValueError("Illegal memory read: addr=0x%08x length=0x%x", addr, length);
	return -2;
}

int PhysicalMemory::write(uint32_t addr, const void *data, uint32_t length) {
	uint32_t end = addr + length - 1;
	
	if (prevRange && prevRange->contains(addr, end)) {
		memcpy((char *)prevRange->buffer + addr - prevRange->start, data, length);
		return 0;
	}
	
	for (PhysicalRange *r : ranges) {
		if (r->contains(addr, end)) {
			memcpy((char *)r->buffer + addr - r->start, data, length);
			prevRange = r;
			return 0;
		}
	}
	
	for (SpecialRange *r : specialRanges) {
		if (r->contains(addr, end)) {
			return r->writeCB(addr, data, length) ? 0 : -1;
		}
	}
	
	ValueError("Illegal memory write: addr=0x%08x length=0x%x", addr, length);
	return -2;
}

int PhysicalMemory::read(uint32_t addr, uint8_t *value) { return readTmpl<uint8_t>(addr, value); }
int PhysicalMemory::read(uint32_t addr, uint16_t *value) { return readTmpl<uint16_t>(addr, value); }
int PhysicalMemory::read(uint32_t addr, uint32_t *value) { return readTmpl<uint32_t>(addr, value); }
int PhysicalMemory::read(uint32_t addr, uint64_t *value) { return readTmpl<uint64_t>(addr, value); }
int PhysicalMemory::write(uint32_t addr, uint8_t value) { return writeTmpl<uint8_t>(addr, value); }
int PhysicalMemory::write(uint32_t addr, uint16_t value) { return writeTmpl<uint16_t>(addr, value); }
int PhysicalMemory::write(uint32_t addr, uint32_t value) { return writeTmpl<uint32_t>(addr, value); }
int PhysicalMemory::write(uint32_t addr, uint64_t value) { return writeTmpl<uint64_t>(addr, value); }

template <class T>
int PhysicalMemory::readTmpl(uint32_t addr, T *value) {
	uint32_t end = addr + sizeof(T) - 1;
	
	if (prevRange && prevRange->contains(addr, end)) {
		*value = *(T *)((char *)prevRange->buffer + addr - prevRange->start);
		return 0;
	}
	
	for (PhysicalRange *r : ranges) {
		if (r->contains(addr, end)) {
			*value = *(T *)((char *)r->buffer + addr - r->start);
			prevRange = r;
			return 0;
		}
	}
	
	for (SpecialRange *r : specialRanges) {
		if (r->contains(addr, end)) {
			return r->readCB(addr, value, sizeof(T)) ? 0 : -1;
		}
	}
	
	ValueError("Illegal memory read: addr=0x%08x length=0x%x", addr, sizeof(T));
	return -2;
}

template <class T>
int PhysicalMemory::writeTmpl(uint32_t addr, T value) {
	uint32_t end = addr + sizeof(T) - 1;
	
	if (prevRange && prevRange->contains(addr, end)) {
		*(T *)((char *)prevRange->buffer + addr - prevRange->start) = value;
		return 0;
	}
	
	for (PhysicalRange *r : ranges) {
		if (r->contains(addr, end)) {
			*(T *)((char *)r->buffer + addr - r->start) = value;
			prevRange = r;
			return 0;
		}
	}
	
	for (SpecialRange *r : specialRanges) {
		if (r->contains(addr, end)) {
			return r->writeCB(addr, &value, sizeof(T)) ? 0 : -1;
		}
	}
	
	ValueError("Illegal memory write: addr=0x%08x length=0x%x", addr, sizeof(T));
	return -2;
}
