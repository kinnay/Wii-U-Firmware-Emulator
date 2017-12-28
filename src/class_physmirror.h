
#pragma once

#include "interface_physmem.h"
#include "interface_virtmem.h"
#include <cstdint>

class PhysicalMirror : public IPhysicalMemory {
	public:
	PhysicalMirror(IPhysicalMemory *physmem, IVirtualMemory *translator);
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
	IPhysicalMemory *physmem;
	IVirtualMemory *translator;
};
