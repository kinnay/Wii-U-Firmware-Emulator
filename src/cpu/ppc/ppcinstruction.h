
#pragma once

#include <cstdint>


class PPCProcessor;

struct PPCInstruction {
	uint32_t value;
	
	inline int opcode() { return value >> 26; }
	inline int opcode2() { return (value >> 1) & 0x3FF; }
	inline int opcode3() { return (value >> 1) & 0x1F; }
	inline int rD() { return (value >> 21) & 0x1F; }
	inline int rS() { return (value >> 21) & 0x1F; }
	inline int rA() { return (value >> 16) & 0x1F; }
	inline int rB() { return (value >> 11) & 0x1F; }
	inline int rC() { return (value >> 6) & 0x1F; }
	inline int crbD() { return (value >> 21) & 0x1F; }
	inline int crbA() { return (value >> 16) & 0x1F; }
	inline int crbB() { return (value >> 11) & 0x1F; }
	inline int crfD() { return (value >> 23) & 7; }
	inline int crm() { return (value >> 12) & 0xFF; }
	inline int spr() {
		return ((value >> 16) & 0x1F) | ((value >> 6) & 0x3E0);
	}
	inline int sr() { return (value >> 16) & 0xF; }
	inline int sh() { return (value >> 11) & 0x1F; }
	inline int mb() { return (value >> 6) & 0x1F; }
	inline int me() { return (value >> 1) & 0x1F; }
	inline int bo() { return (value >> 21) & 0x1F; }
	inline int bi() { return (value >> 16) & 0x1F; }
	inline int bd() {
		int val = value & 0xFFFC;
		if (val & 0x8000) {
			val -= 0x10000;
		}
		return val;
	}
	inline int uimm() { return value & 0xFFFF; }
	inline int simm() {
		int imm = value & 0xFFFF;
		if (imm & 0x8000) {
			imm -= 0x10000;
		}
		return imm;
	}
	inline int d() { return simm(); }
	inline int li() {
		int val = value & 0x03FFFFFC;
		if (val & 0x02000000) {
			val -= 0x04000000;
		}
		return val;
	}
	inline bool rc() { return value & 1; }
	inline bool lk() { return value & 1; }
	inline bool aa() { return (value >> 1) & 1; }
	
	inline int ps_w() { return (value >> 15) & 1; }
	inline int ps_d() {
		int val = value & 0xFFF;
		if (val & 0x800) {
			val -= 0x1000;
		}
		return val;
	}
	inline bool ps_i() { return (value >> 12) & 7; }
	
	void execute(PPCProcessor *cpu);
};
