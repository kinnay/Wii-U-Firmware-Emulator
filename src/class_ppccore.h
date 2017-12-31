
#pragma once

#include "class_ppclock.h"
#include "class_bits.h"
#include <functional>
#include <cstdint>

class PPCCore {
	public:
	union FPR {
		struct {
			float ps0;
			float ps1;
		};
		
		double dbl;
	};
	
	enum SPR {
		XER = 1,
		LR = 8,
		CTR = 9,
		DSISR = 18,
		DAR = 19,
		SRR0 = 26,
		SRR1 = 27,
		UGQR0 = 896,
		UGQR1 = 897,
		UGQR2 = 898,
		UGQR3 = 899,
		UGQR4 = 900,
		UGQR5 = 901,
		UGQR6 = 902,
		UGQR7 = 903,
		GQR0 = 912,
		GQR1 = 913,
		GQR2 = 914,
		GQR3 = 915,
		GQR4 = 916,
		GQR5 = 917,
		GQR6 = 918,
		GQR7 = 919,
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
	
	enum ExceptionType {
		DSI,
		ISI,
		ExternalInterrupt,
		Decrementer,
		SystemCall
	};
	
	typedef std::function<bool(SPR spr, uint32_t *value)> SPRReadCB;
	typedef std::function<bool(SPR spr, uint32_t value)> SPRWriteCB;
	typedef std::function<bool(uint32_t value)> MSRWriteCB;
	typedef std::function<bool(int index, uint32_t *value)> SRReadCB;
	typedef std::function<bool(int index, uint32_t value)> SRWriteCB;
	
	PPCLockMgr *lockMgr;
	
	uint32_t regs[32];
	FPR fprs[32];
	uint32_t gqrs[8];
	
	uint32_t pc;
	Bits cr;
	uint32_t lr;
	uint32_t ctr;
	Bits xer;
	
	uint32_t msr;
	uint32_t srr0;
	uint32_t srr1;
	uint32_t dsisr;
	uint32_t dar;
	
	PPCCore(PPCLockMgr *lockMgr);
	
	bool setSpr(SPR spr, uint32_t value);
	bool getSpr(SPR spr, uint32_t *value);
	bool setMsr(uint32_t value);
	bool setSr(int index, uint32_t value);
	bool getSr(int index, uint32_t *value);
	
	void setSprReadCB(SPRReadCB callback);
	void setSprWriteCB(SPRWriteCB callback);
	void setMsrWriteCB(MSRWriteCB callback);
	void setSrReadCB(SRReadCB callback);
	void setSrWriteCB(SRWriteCB callback);
	
	void triggerException(ExceptionType type);
	
	private:
	SPRReadCB sprReadCB;
	SPRWriteCB sprWriteCB;
	MSRWriteCB msrWriteCB;
	SRReadCB srReadCB;
	SRWriteCB srWriteCB;
};
