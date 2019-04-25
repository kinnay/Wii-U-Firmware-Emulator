
#pragma once

#include "class_range.h"
#include <vector>
#include <cstdint>
#include <functional>

typedef std::function<bool(uint32_t addr, void *data, uint32_t length)> ReadCB;
typedef std::function<bool(uint32_t addr, const void *data, uint32_t length)> WriteCB;

class SpecialRange : public Range {
public:
	SpecialRange(uint32_t start, uint32_t end, ReadCB, WriteCB);
	
	ReadCB readCB;
	WriteCB writeCB;
};

class PhysicalRange : public Range {
public:
	PhysicalRange(uint32_t start, uint32_t end, char *buffer);

	char *buffer;
};

class PhysicalMemory {
public:
	PhysicalMemory();
	~PhysicalMemory();
	
	bool addRange(uint32_t start, uint32_t end);
	bool addSpecial(uint32_t start, uint32_t end, ReadCB readCB, WriteCB writeCB);
	
	int read(uint32_t addr, void *data, uint32_t length);
	int write(uint32_t addr, const void *data, uint32_t length);
	
	template <class T>
	int read(uint32_t addr, T *value) {
		uint32_t end = addr + sizeof(T) - 1;
		
		if (end >= addr) {
			if (prevRange && prevRange->contains(addr, end)) {
				*value = *(T *)(prevRange->buffer + addr - prevRange->start);
				return 0;
			}
			
			for (PhysicalRange &r : ranges) {
				if (r.contains(addr, end)) {
					*value = *(T *)(r.buffer + addr - r.start);
					prevRange = &r;
					return 0;
				}
			}
			
			for (SpecialRange &r : specialRanges) {
				if (r.contains(addr, end)) {
					return r.readCB(addr, value, sizeof(T)) ? 0 : -1;
				}
			}
		}
		
		ValueError("Illegal memory read: addr=0x%08x length=0x%x", addr, sizeof(T));
		return -2;
	}

	template <class T>
	int write(uint32_t addr, T value) {
		uint32_t end = addr + sizeof(T) - 1;
		
		if (end >= addr) {
			if (prevRange && prevRange->contains(addr, end)) {
				*(T *)(prevRange->buffer + addr - prevRange->start) = value;
				return 0;
			}
			
			for (PhysicalRange &r : ranges) {
				if (r.contains(addr, end)) {
					*(T *)(r.buffer + addr - r.start) = value;
					prevRange = &r;
					return 0;
				}
			}
			
			for (SpecialRange &r : specialRanges) {
				if (r.contains(addr, end)) {
					return r.writeCB(addr, &value, sizeof(T)) ? 0 : -1;
				}
			}
		}
		
		ValueError("Illegal memory write: addr=0x%08x length=0x%x", addr, sizeof(T));
		return -2;
	}
	
	template<int len> int read(uint32_t addr, void *value);
	template<int len> int write(uint32_t addr, void *value);
	
	template<> int read<1>(uint32_t addr, void *value) { return read(addr, (uint8_t *)value); }
	template<> int read<2>(uint32_t addr, void *value) { return read(addr, (uint16_t *)value); }
	template<> int read<4>(uint32_t addr, void *value) { return read(addr, (uint32_t *)value); }
	template<> int read<8>(uint32_t addr, void *value) { return read(addr, (uint64_t *)value); }
	
	template<> int write<1>(uint32_t addr, void *value) { return write(addr, *(uint8_t *)value); }
	template<> int write<2>(uint32_t addr, void *value) { return write(addr, *(uint16_t *)value); }
	template<> int write<4>(uint32_t addr, void *value) { return write(addr, *(uint32_t *)value); }
	template<> int write<8>(uint32_t addr, void *value) { return write(addr, *(uint64_t *)value); }
	
private:
	std::vector<SpecialRange> specialRanges;
	std::vector<PhysicalRange> ranges;
	PhysicalRange *prevRange;
	
	bool checkOverlap(uint32_t start, uint32_t end);
	bool checkSize(uint32_t start, uint32_t end);
};
