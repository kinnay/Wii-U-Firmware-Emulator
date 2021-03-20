
#include "physicalmemory.h"
#include "hardware.h"

#include "common/exceptions.h"

#include <cstring>


bool isHardware(uint32_t addr) {
	return (addr & 0xFE400000) == 0x0C000000 || (addr & 0xFE000000) == 0xD0000000;
}


PhysicalMemory::PhysicalMemory(Hardware *hardware) {
	this->hardware = hardware;
	
	mem = (char *)malloc(0x100000000);
}

PhysicalMemory::~PhysicalMemory() {
	free(mem);
}

template <>
std::string PhysicalMemory::read(uint32_t addr) {
	std::string value;
	while (true) {
		uint8_t c = read<uint8_t>(addr++);
		if (!c) {
			break;
		}
		value += c;
	}
	return value;
}

template <>
void PhysicalMemory::write(uint32_t addr, std::string value) {
	for (uint8_t c : value) {
		write<uint8_t>(addr++, c);
	}
	write<uint8_t>(addr, 0);
}

void PhysicalMemory::read(uint32_t addr, void *buffer, size_t size) {
	if (isHardware(addr)) {
		if (size == 2) *(uint16_t *)buffer = Endian::swap16(hardware->read<uint16_t>(addr));
		else if (size == 4) *(uint32_t *)buffer = Endian::swap32(hardware->read<uint32_t>(addr));
		else {
			Logger::warning("Invalid physical memory read: 0x%08X (%i bytes)", addr, size);
		}
	}
	else {
		memcpy(buffer, mem + addr, size);
	}
}

void PhysicalMemory::write(uint32_t addr, const void *buffer, size_t size) {
	if (isHardware(addr)) {
		if (size == 2) hardware->write<uint16_t>(addr, Endian::swap16(*(uint16_t *)buffer));
		else if (size == 4) hardware->write<uint32_t>(addr, Endian::swap32(*(uint32_t *)buffer));
		else {
			Logger::warning("Invalid physical memory write: 0x%08X (%i bytes)", addr, size);
		}
	}
	else {
		memcpy(mem + addr, buffer, size);
	}
}

Buffer PhysicalMemory::read(uint32_t addr, size_t size) {
	Buffer buffer(size);
	read(addr, buffer.get(), size);
	return buffer;
}

void PhysicalMemory::write(uint32_t addr, Buffer buffer) {
	write(addr, buffer.get(), buffer.size());
}
