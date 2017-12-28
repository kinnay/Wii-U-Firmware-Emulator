
#pragma once

#include "class_bits.h"
#include <cstdint>

class ARMCore {
	public:
	enum Register {
		R0, R1, R2, R3, R4, R5, R6,
		R7, R8, R9, R10, R11, R12,
		SP, LR, PC
	};
	
	enum Status : uint32_t {
		N = 1u << 31,
		Z = 1 << 30,
		C = 1 << 29,
		V = 1 << 28,
		I = 1 << 7,
		F = 1 << 6,
		T = 1 << 5
	};
	
	enum Mode {
		User = 16,
		FIQ = 17,
		IRQ = 18,
		SVC = 19,
		Abort = 23,
		Undefined = 27,
		System = 31
	};
	
	enum ExceptionType {
		UndefinedInstruction,
		DataAbort,
		InterruptRequest
	};
	
	ARMCore();
	void setMode(Mode mode);
	void setThumb(bool thumb);
	void triggerException(ExceptionType type);
	
	void readModeRegs();
	void writeModeRegs();
	
	Mode mode;
	uint32_t regs[16];
	uint32_t regsUser[16];
	Bits cpsr;
	Bits spsr;
	bool thumb;
	
	private:
	uint32_t regsFiq[7];
	uint32_t regsIrq[2];
	uint32_t regsSvc[2];
	uint32_t regsAbt[2];
	uint32_t regsUnd[2];
	Bits spsrFiq;
	Bits spsrIrq;
	Bits spsrSvc;
	Bits spsrAbt;
	Bits spsrUnd;
};
