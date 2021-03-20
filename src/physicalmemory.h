
#pragma once

#include "common/endian.h"
#include "common/buffer.h"
#include "hardware.h"

#include <string>

#include <cstdint>
#include <cstddef>


bool isHardware(uint32_t addr);


class PhysicalMemory {
public:
	PhysicalMemory(Hardware *hardware);
	~PhysicalMemory();
	
	template <class T>
	T read(uint32_t addr) {
		if (isHardware(addr)) {
			return hardware->read<T>(addr);
		}
		T value = *(T *)(mem + addr);
		Endian::swap(&value);
		return value;
	}
	
	template <class T>
	void write(uint32_t addr, T value) {
		if (isHardware(addr)) {
			hardware->write<T>(addr, value);
		}
		else {
			Endian::swap(&value);
			*(T *)(mem + addr) = value;
		}
	}
	
	void read(uint32_t addr, void *buffer, size_t size);
	void write(uint32_t addr, const void *buffer, size_t size);
	
	Buffer read(uint32_t addr, size_t size);
	void write(uint32_t addr, Buffer data);
	
private:
	char *mem;
	
	Hardware *hardware;
};

template <> std::string PhysicalMemory::read<std::string>(uint32_t addr);
template <> void PhysicalMemory::write<std::string>(uint32_t addr, std::string value);
