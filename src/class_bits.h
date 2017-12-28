
#pragma once

#include <cstdint>

class Bits {
	public:
	uint32_t value;
	
	inline void set(uint32_t mask, int state=true) {
		if (state) value |= mask;
		else value &= ~mask;
	}
	
	inline bool get(uint32_t mask) {
		return (value & mask) != 0;
	}
	
	inline operator uint32_t() {
		return value;
	}
	
	inline uint32_t operator =(uint32_t value) {
		this->value = value;
		return value;
	}
};
