
#include "class_ppccore.h"
#include "errors.h"
#include <cstdint>

PPCCore::PPCCore(PPCLockMgr *lockMgr)
	: lockMgr(lockMgr), sprReadCB(0), sprWriteCB(0), msrWriteCB(0),
	  srReadCB(0), srWriteCB(0), decrementerPending(false),iciPending(false)
{
	for (int i = 0; i < 8; i++) {
		gqrs[i] = 0;
	}
	fpscr = 0;
}

bool PPCCore::triggerException(ExceptionType type) {
	if (!(msr & 0x8000)) {
		if (type == ExternalInterrupt) return true;
		if (type == Decrementer) {
			decrementerPending = true;
			return true;
		}
		if (type == ICI) {
			iciPending = true;
			return true;
		}
	}
	
	srr0 = pc;
	srr1 = (msr & 0xFF73) | 2; //Recoverable exception
	if (!setMsr(msr & ~0x4EF70)) return false;

	if (type == DSI) {
		srr0 = pc - 4;
		pc = 0xFFF00300;
	}
	else if (type == ISI) pc = 0xFFF00400;
	else if (type == ExternalInterrupt) pc = 0xFFF00500;
	else if (type == Decrementer) pc = 0xFFF00900;
	else if (type == SystemCall) pc = 0xFFF00C00;
	else if (type == ICI) pc = 0xFFF01700;
	return true;
}

bool PPCCore::setSpr(SPR spr, uint32_t value) {
	if (spr == LR) lr = value;
	else if (spr == CTR) ctr = value;
	else if (spr == XER) xer = value;
	else if (spr == SPRG0) sprg0 = value;
	else if (spr == SPRG1) sprg1 = value;
	else if (spr == SPRG2) sprg2 = value;
	else if (spr == SPRG3) sprg3 = value;
	else if (spr == UPIR) upir = value;
	else if (spr == THRM3) thrm3 = value;
	else if (spr == TBL) tb = (tb & 0xFFFFFFFF00000000) | value;
	else if (spr == TBU) tb = (tb & 0x00000000FFFFFFFF) | ((uint64_t)value << 32);
	else if (spr == DSISR) dsisr = value;
	else if (spr == DAR) dar = value;
	else if (spr == SRR0) srr0 = value;
	else if (spr == SRR1) srr1 = value;
	else if (UGQR0 <= spr && spr <= UGQR7) gqrs[spr - UGQR0] = value;
	else if (GQR0 <= spr && spr <= GQR7) gqrs[spr - GQR0] = value;
	else {
		if (!sprWriteCB) {
			RuntimeError("No SPR write callback installed");
			return false;
		}
		return sprWriteCB(spr, value);
	}
	return true;
}

bool PPCCore::getSpr(SPR spr, uint32_t *value) {
	if (spr == LR) *value = lr;
	else if (spr == CTR) *value = ctr;
	else if (spr == XER) *value = xer;
	else if (spr == SPRG0) *value = sprg0;
	else if (spr == SPRG1) *value = sprg1;
	else if (spr == SPRG2) *value = sprg2;
	else if (spr == SPRG3) *value = sprg3;
	else if (spr == UPIR) *value = upir;
	else if (spr == THRM3) *value = thrm3;
	else if (spr == UTBL) *value = (uint32_t)tb;
	else if (spr == UTBU) *value = tb >> 32;
	else if (spr == DSISR) *value = dsisr;
	else if (spr == DAR) *value = dar;
	else if (spr == SRR0) *value = srr0;
	else if (spr == SRR1) *value = srr1;
	else if (UGQR0 <= spr && spr <= UGQR7) *value = gqrs[spr - UGQR0];
	else if (GQR0 <= spr && spr <= GQR7) *value = gqrs[spr - GQR0];
	else {
		if (!sprReadCB) {
			RuntimeError("No SPR read callback installed");
			return false;
		}
		return sprReadCB(spr, value);
	}
	return true;
}

bool PPCCore::setMsr(uint32_t value) {
	msr = value;
	if (msrWriteCB) {
		if (!msrWriteCB(value)) return false;
	}
	
	if (msr & 0x8000) {
		if (decrementerPending) {
			decrementerPending = false;
			return triggerException(Decrementer);
		}
		if (iciPending) {
			iciPending = false;
			return triggerException(ICI);
		}
	}
	return true;
}

bool PPCCore::setSr(int index, uint32_t value) {
	if (!srWriteCB) {
		RuntimeError("No SR write callback installed");
		return false;
	}
	return srWriteCB(index, value);
}

bool PPCCore::getSr(int index, uint32_t *value) {
	if (!srReadCB) {
		RuntimeError("No SR read callback installed");
		return false;
	}
	return srReadCB(index, value);
}


void PPCCore::setSprReadCB(SPRReadCB callback) { sprReadCB = callback; }
void PPCCore::setSprWriteCB(SPRWriteCB callback) { sprWriteCB = callback; }
void PPCCore::setMsrWriteCB(MSRWriteCB callback) { msrWriteCB = callback; }
void PPCCore::setSrReadCB(SRReadCB callback) { srReadCB = callback; }
void PPCCore::setSrWriteCB(SRWriteCB callback) { srWriteCB = callback; }
