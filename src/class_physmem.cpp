
#include "class_physmem.h"
#include "class_range.h"
#include "errors.h"
#include <cstdint>

SpecialRange::SpecialRange(uint32_t start, uint32_t end, ReadCB readCB, WriteCB writeCB)
	: Range(start, end), readCB(readCB), writeCB(writeCB) {}

PhysicalMemory::~PhysicalMemory() {
	for (Range *r : ranges) {
		delete r;
	}
	
	for (SpecialRange *r : specialRanges) {
		delete r;
	}
	
	for (void *buffer : buffers) {
		delete buffer;
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
	
	Range *range = new Range(start, end);
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
	
	ranges.push_back(range);
	buffers.push_back(buffer);
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
	
	int i = 0;
	for (Range *r : ranges) {
		if (r->contains(addr, end)) {
			void *buffer = buffers[i];
			memcpy(data, (char *)buffer + addr - r->start, length);
			return 0;
		}
		i++;
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
	
	int i = 0;
	for (Range *r : ranges) {
		if (r->contains(addr, end)) {
			void *buffer = buffers[i];
			memcpy((char *)buffer + addr - r->start, data, length);
			return 0;
		}
		i++;
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
	
	int i = 0;
	for (Range *r : ranges) {
		if (r->contains(addr, end)) {
			void *buffer = buffers[i];
			*value = *(T *)((char *)buffer + addr - r->start);
			return 0;
		}
		i++;
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
	
	int i = 0;
	for (Range *r : ranges) {
		if (r->contains(addr, end)) {
			void *buffer = buffers[i];
			*(T *)((char *)buffer + addr - r->start) = value;
			return 0;
		}
		i++;
	}
	
	for (SpecialRange *r : specialRanges) {
		if (r->contains(addr, end)) {
			return r->writeCB(addr, &value, sizeof(T)) ? 0 : -1;
		}
	}
	
	ValueError("Illegal memory write: addr=0x%08x length=0x%x", addr, sizeof(T));
	return -2;
}
