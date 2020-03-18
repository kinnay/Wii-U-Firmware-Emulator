
#pragma once

#include <cstdint>

struct ARMThumbInstr;
class ARMProcessor;

struct ARMThumbInstr {
	public:
	uint16_t value;
	
	inline bool b() { return (value >> 12) & 1; }
	inline bool h() { return (value >> 11) & 1; }
	inline bool l() { return (value >> 11) & 1; }
	inline bool s() { return (value >> 10) & 1; }
	inline bool i() { return (value >> 10) & 1; }
	inline bool r() { return (value >> 8) & 1; }
	
	void execute(ARMProcessor *cpu);
};
