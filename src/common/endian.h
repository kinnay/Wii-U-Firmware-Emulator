
#pragma once

#include <cstdint>

class Endian {
public:
	enum Type {
		Little, Big
	};

	static inline uint8_t swap8(uint8_t value) { return value; }
	static inline uint16_t swap16(uint16_t value) {
		return (value << 8) | (value >> 8);
	}
	static inline uint32_t swap32(uint32_t value) {
		return (swap16(value & 0xFFFF) << 16) | swap16(value >> 16);
	}
	static inline uint64_t swap64(uint64_t value) {
		return ((uint64_t)swap32(value & 0xFFFFFFFF) << 32) | swap32(value >> 32);
	}
	
	template <int N>
	static void swap(void *value);
	
	template <class T>
	static void swap(T *value) {
		swap<sizeof(T)>(value);
	}
};


template <>
inline void Endian::swap<1>(void *value) {}

template <>
inline void Endian::swap<2>(void *value) {
	*(uint16_t *)value = swap16(*(uint16_t *)value);
}

template <>
inline void Endian::swap<4>(void *value) {
	*(uint32_t *)value = swap32(*(uint32_t *)value);
}

template <>
inline void Endian::swap<8>(void *value) {
	*(uint64_t *)value = swap64(*(uint64_t *)value);
}
