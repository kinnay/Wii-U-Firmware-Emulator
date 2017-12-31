
#include "class_ppccore.h"
#include "errors.h"
#include <cstdint>

PPCCore::PPCCore(PPCLockMgr *lockMgr)
	: lockMgr(lockMgr), sprReadCB(0), sprWriteCB(0), msrWriteCB(0),
	  srReadCB(0), srWriteCB(0)
{
	for (int i = 0; i < 8; i++) {
		gqrs[i] = 0;
	}
}

void PPCCore::triggerException(ExceptionType type) {
	if (!(msr & 0x8000) && (type == ExternalInterrupt || type == Decrementer)) {
		return;
	}
	
	srr0 = pc;
	srr1 = (msr & 0xFF73) | 2; //Recoverable exception
	setMsr(msr & ~0x4EF70);

	if (type == DSI) {
		srr0 = pc - 4;
		pc = 0xFFF00300;
	}
	else if (type == ISI) pc = 0xFFF00400;
	else if (type == ExternalInterrupt) pc = 0xFFF00500;
	else if (type == Decrementer) pc = 0xFFF00900;
	else if (type == SystemCall) pc = 0xFFF00C00;
}

bool PPCCore::setSpr(SPR spr, uint32_t value) {
	if (spr == XER) xer = value;
	else if (spr == LR) lr = value;
	else if (spr == CTR) ctr = value;
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
	if (spr == XER) *value = xer;
	else if (spr == LR) *value = lr;
	else if (spr == CTR) *value = ctr;
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
