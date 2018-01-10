
#pragma once

#include "interface_physmem.h"
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
	PhysicalRange(uint32_t start, uint32_t end);

	void *buffer;
};

class PhysicalMemory : public IPhysicalMemory {
	public:
	~PhysicalMemory();
	bool addRange(uint32_t start, uint32_t end);
	bool addSpecial(uint32_t start, uint32_t end, ReadCB readCB, WriteCB writeCB);
	int read(uint32_t addr, void *data, uint32_t length);
	int write(uint32_t addr, const void *data, uint32_t length);
	int read(uint32_t addr, uint8_t *value);
	int read(uint32_t addr, uint16_t *value);
	int read(uint32_t addr, uint32_t *value);
	int read(uint32_t addr, uint64_t *value);
	int write(uint32_t addr, uint8_t value);
	int write(uint32_t addr, uint16_t value);
	int write(uint32_t addr, uint32_t value);
	int write(uint32_t addr, uint64_t value);
	
	private:
	template <class T> int readTmpl(uint32_t addr, T *value);
	template <class T> int writeTmpl(uint32_t addr, T value);
	
	std::vector<SpecialRange *> specialRanges;
	std::vector<PhysicalRange *> ranges;
	
	bool checkOverlap(uint32_t start, uint32_t end);
};
