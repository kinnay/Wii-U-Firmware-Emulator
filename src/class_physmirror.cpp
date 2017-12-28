
#include "class_physmirror.h"
#include <cstdint>

PhysicalMirror::PhysicalMirror(IPhysicalMemory *physmem, IVirtualMemory *translator)
	: physmem(physmem), translator(translator) {}
	
int PhysicalMirror::read(uint32_t addr, void *data, uint32_t length) {
	if (!translator->translate(&addr, length, IVirtualMemory::DataRead)) return -2;
	return physmem->read(addr, data, length);
}

int PhysicalMirror::write(uint32_t addr, const void *data, uint32_t length) {
	if (!translator->translate(&addr, length, IVirtualMemory::DataWrite)) return -2;
	return physmem->write(addr, data, length);
}

int PhysicalMirror::read(uint32_t addr, uint8_t *value) {
	if (!translator->translate(&addr, 1, IVirtualMemory::DataRead)) return -2;
	return physmem->read(addr, value);
}

int PhysicalMirror::read(uint32_t addr, uint16_t *value) {
	if (!translator->translate(&addr, 2, IVirtualMemory::DataRead)) return -2;
	return physmem->read(addr, value);
}

int PhysicalMirror::read(uint32_t addr, uint32_t *value) {
	if (!translator->translate(&addr, 4, IVirtualMemory::DataRead)) return -2;
	return physmem->read(addr, value);
}

int PhysicalMirror::read(uint32_t addr, uint64_t *value) {
	if (!translator->translate(&addr, 8, IVirtualMemory::DataRead)) return -2;
	return physmem->read(addr, value);
}

int PhysicalMirror::write(uint32_t addr, uint8_t value) {
	if (!translator->translate(&addr, 1, IVirtualMemory::DataWrite)) return -2;
	return physmem->write(addr, value);
}

int PhysicalMirror::write(uint32_t addr, uint16_t value) {
	if (!translator->translate(&addr, 2, IVirtualMemory::DataWrite)) return -2;
	return physmem->write(addr, value);
}

int PhysicalMirror::write(uint32_t addr, uint32_t value) {
	if (!translator->translate(&addr, 4, IVirtualMemory::DataWrite)) return -2;
	return physmem->write(addr, value);
}

int PhysicalMirror::write(uint32_t addr, uint64_t value) {
	if (!translator->translate(&addr, 8, IVirtualMemory::DataWrite)) return -2;
	return physmem->write(addr, value);
}
