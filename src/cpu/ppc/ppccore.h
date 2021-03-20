
#pragma once

#include "common/bits.h"
#include "config.h"

#include <map>

#include <cstdint>


class PPCCore {
public:
	union FPR {
		struct {
			float ps1;
			float ps0;
		};
		struct {
			int32_t iw1;
			int32_t iw0;
		};
		struct {
			uint32_t uw1;
			uint32_t uw0;
		};
		double dbl;
	};
	
	enum SPR {
		XER = 1,
		DSISR = 2,
		DAR = 3,
		DEC = 6,
		LR = 8,
		CTR = 9,
		SRR0 = 0xA,
		SRR1 = 0xB,
		SDR1 = 0xC,
		TBL = 0x8C,
		TBU = 0x8D,
		PVR = 0x8F,
		
		IBAT0U = 0x100,
		IBAT0L = 0x101,
		
		DBAT0U = 0x108,
		DBAT0L = 0x109,
		
		IBAT4U = 0x110,
		IBAT4L = 0x111,
		
		DBAT4U = 0x118,
		DBAT4L = 0x119,
		
		GQR0 = 0x1C0,
		
		HID2 = 0x1C8,
		
		WPAR = 0x1C9,
		
		HID5 = 0x1D0,
		
		PCSR = 0x1D2,
		SCR = 0x1D3,
		CAR = 0x1D4,
		BCR = 0x1D5,
		
		WPSAR = 0x1D6,
		
		HID0 = 0x1F0,
		HID1 = 0x1F1,
		
		IABR = 0x1F2,
		
		HID4 = 0x1F3,
		
		DABR = 0x1F5,
		
		PIR = 0x1FF
	};
	
	enum ExceptionType {
		SystemReset = 1,
		DSI = 3,
		ISI,
		ExternalInterrupt,
		Decrementer = 9,
		SystemCall = 12,
		ICI = 23
	};
	
	enum Status : uint32_t {
		LT = 1u << 31,
		GT = 1 << 30,
		EQ = 1 << 29
	};
	
	enum CarryStatus : uint32_t {
		SO = 1u << 31,
		OV = 1 << 30,
		CA = 1 << 29
	};
	
	void reset();
	
	void triggerException(ExceptionType type);
	void checkPendingExceptions();
	
	bool getCarry();
	void setCarry(bool carry);
	
	Bits cr;
	uint32_t pc;
	uint32_t regs[32];
	uint32_t sprs[0x200];
	
	FPR fprs[32];
	
	uint32_t sr[16];
	
	uint32_t msr;
	
	uint32_t fpscr;
	
	#if STATS
	uint64_t dsiExceptions;
	uint64_t isiExceptions;
	uint64_t externalInterrupts;
	uint64_t decrementerInterrupts;
	uint64_t systemCalls;
	#endif
	
	#if METRICS
	std::map<uint32_t, uint64_t> syscallIds;
	#endif
	
private:
	bool isMaskable(ExceptionType type);
	
	bool externalInterruptPending;
	bool decrementerPending;
	bool iciPending;
};
