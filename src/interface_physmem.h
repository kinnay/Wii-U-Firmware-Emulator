
#pragma once

#include <cstdint>

class IPhysicalMemory {
	public:
	//Can't do virtual templates, unfortunately
	virtual int read(uint32_t addr, void *data, uint32_t length) = 0;
	virtual int write(uint32_t addr, const void *data, uint32_t length) = 0;
	virtual int read(uint32_t addr, uint8_t *value) = 0;
	virtual int read(uint32_t addr, uint16_t *value) = 0;
	virtual int read(uint32_t addr, uint32_t *value) = 0;
	virtual int read(uint32_t addr, uint64_t *value) = 0;
	virtual int write(uint32_t addr, uint8_t value) = 0;
	virtual int write(uint32_t addr, uint16_t value) = 0;
	virtual int write(uint32_t addr, uint32_t value) = 0;
	virtual int write(uint32_t addr, uint64_t value) = 0;
	
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
};
