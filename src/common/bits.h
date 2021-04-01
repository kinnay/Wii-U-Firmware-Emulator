
#pragma once

#include "common/exceptions.h"

#include <cstdint>

template<class T>
class Bits {
public:
	Bits() {
		value = 0;
	}
	
	inline T getfield(T mask) const {
		return value & mask;
	}
	
	inline void setfield(T mask, T value) {
		this->value = (this->value & ~mask) | value;
	}
	
	inline bool get(T mask) const {
		return value & mask;
	}
	
	inline void set(T mask, bool enable) {
		if (enable) value |= mask;
		else value &= ~mask;
	}

	inline T operator =(T value) {
		this->value = value;
		return value;
	}
	
	inline operator T() const {
		return value;
	}
	
private:
	T value;
};
