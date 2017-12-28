
#pragma once

#include "interface_virtmem.h"
#include "range.h"
#include <vector>
#include <cstdint>

class MemoryRange : public Range {
	public:
	MemoryRange(uint32_t virt, uint32_t phys, uint32_t length);
	
	uint32_t phys;
};

class VirtualMemory : public IVirtualMemory {
	public:
	~VirtualMemory();
	bool addRange(uint32_t virt, uint32_t phys, uint32_t length);
	bool translate(uint32_t *addr, uint32_t length, Access type);
	
	private:
	std::vector<MemoryRange *> ranges;
};
