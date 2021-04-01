
#pragma once

#include "common/bits.h"

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
		I = 1 << 7, // Disables IRQ
		F = 1 << 6, // Disables FIQ
		T = 1 << 5, // Thumb mode
		Mode = 0x1F
	};
	
	enum ProcessorMode {
		User = 16,
		FIQ = 17,
		IRQ = 18,
		SVC = 19,
		Abort = 23,
		Undefined = 27,
		System = 31
	};
	
	enum ExceptionType {
		Reset,
		UndefinedInstruction,
		SoftwareInterrupt,
		PrefetchAbort,
		DataAbort,
		InterruptRequest,
		FastInterrupt
	};
	
	ARMCore();
	
	void reset();
	
	bool isThumb();
	void setThumb(bool thumb);
	
	ProcessorMode getMode();
	void setMode(ProcessorMode mode);
	
	void readModeRegs();
	void writeModeRegs();
	
	void triggerException(ExceptionType type);
	
	Bits<uint32_t> cpsr;
	Bits<uint32_t> spsr;
	
	uint32_t regs[16];
	uint32_t regsUser[16];
	uint32_t regsFiq[7];
	uint32_t regsIrq[2];
	uint32_t regsSvc[2];
	uint32_t regsAbt[2];
	uint32_t regsUnd[2];
	
	Bits<uint32_t> spsrFiq;
	Bits<uint32_t> spsrIrq;
	Bits<uint32_t> spsrSvc;
	Bits<uint32_t> spsrAbt;
	Bits<uint32_t> spsrUnd;
	
	uint32_t control;
	uint32_t domain;
	uint32_t ttbr;
	uint32_t dfsr;
	uint32_t ifsr;
	uint32_t far;
};
