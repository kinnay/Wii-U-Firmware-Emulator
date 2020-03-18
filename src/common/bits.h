
#pragma once

#include "common/exceptions.h"

#include <cstdint>

class Bits {
public:
	Bits() {
		value = 0;
	}
	
	inline uint32_t get(uint32_t mask) const {
		return value & mask;
	}
	
	inline void setfield(uint32_t mask, uint32_t value) {
		this->value = (this->value & ~mask) | value;
	}
	
	inline void set(uint32_t mask, bool enable) {
		if (enable) value |= mask;
		else value &= ~mask;
	}

	inline uint32_t operator =(uint32_t value) {
		this->value = value;
		return value;
	}
	
	inline operator uint32_t() const {
		return value;
	}
	
private:
	uint32_t value;
};
