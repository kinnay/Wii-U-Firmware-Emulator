
#pragma once

#include <cstdint>

class Endian {
public:
	static void swap(uint8_t *);
	static void swap(uint16_t *);
	static void swap(uint32_t *);
	static void swap(uint64_t *);
	static uint8_t swap8(uint8_t);
	static uint16_t swap16(uint16_t);
	static uint32_t swap32(uint32_t);
	static uint64_t swap64(uint64_t);
	
	template<int len> static void swap(void *value);
	template<> static void swap<1>(void *value) { swap((uint8_t *)value); }
	template<> static void swap<2>(void *value) { swap((uint16_t *)value); }
	template<> static void swap<4>(void *value) { swap((uint32_t *)value); }
	template<> static void swap<8>(void *value) { swap((uint64_t *)value); }
};
