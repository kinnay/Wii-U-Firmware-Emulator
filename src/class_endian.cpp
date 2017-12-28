
#include "class_endian.h"
#include <cstdint>

uint8_t Endian::swap8(uint8_t value) { return value; }
uint16_t Endian::swap16(uint16_t value) { return (value << 8) | (value >> 8); }
uint32_t Endian::swap32(uint32_t value) { return (swap16(value & 0xFFFF) << 16) | swap16(value >> 16); }
uint64_t Endian::swap64(uint64_t value) { return ((uint64_t)swap32(value & 0xFFFFFFFF) << 32) | swap32(value >> 32); }

void Endian::swap(uint8_t *value) {}
void Endian::swap(uint16_t *value) { *value = swap16(*value); }
void Endian::swap(uint32_t *value) { *value = swap32(*value); }
void Endian::swap(uint64_t *value) { *value = swap64(*value); }

Endian::Type Endian::getSystemEndian() {
	static Type endian = Unknown;
	if (endian == Unknown) {
		uint16_t value = 0xFEFF;
		if (*(uint8_t *)&value == 0xFE) {
			endian = Big;
		}
		else endian = Little;
	}
	return endian;
}
