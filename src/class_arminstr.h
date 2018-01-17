
#pragma once

#include <cstdint>

struct ARMInstruction;
class ARMInterpreter;

struct ARMInstruction {
	enum Cond {
		EQ, NE, CS, CC, MI, PL, VS, VC,
		HI, LS, GE, LT, GT, LE, AL
	};
	
	uint32_t value;
	
	inline int cond() { return value >> 28; }
	inline int opcode() { return (value >> 21) & 0xF; }
	inline int shift() { return (value >> 4) & 0xFF; }
	inline int rotate() { return (value >> 8) & 0xF; }
	inline int offset() {
		int result = value & 0xFFFFFF;
		if (result > 0x800000) {
			result -= 0x1000000;
		}
		return result * 4;
	}
	inline bool link() { return (value >> 24) & 1; }
	
	inline int cpopc() { return (value >> 21) & 7; }
	inline int cp() { return (value >> 5) & 7; }
	
	inline int r0() { return (value >> 16) & 0xF; }
	inline int r1() { return (value >> 12) & 0xF; }
	inline int r2() { return (value >> 8) & 0xF; }
	inline int r3() { return value & 0xF; }
	
	inline bool i() { return (value >> 25) & 1; }
	inline bool p() { return (value >> 24) & 1; }
	inline bool u() { return (value >> 23) & 1; }
	inline bool b() { return (value >> 22) & 1; }
	inline bool r() { return (value >> 22) & 1; }
	inline bool w() { return (value >> 21) & 1; }
	inline bool a() { return (value >> 21) & 1; }
	inline bool l() { return (value >> 20) & 1; }
	inline bool s() { return (value >> 20) & 1; }
	inline bool h() { return (value >> 5) & 1; }
	
	bool execute(ARMInterpreter *cpu);
};
