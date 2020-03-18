
#include "ppccore.h"
#include "common/logger.h"

#include <cstring>


void PPCCore::reset() {
	memset(regs, 0, sizeof(regs));
	memset(fprs, 0, sizeof(fprs));
	memset(sprs, 0, sizeof(sprs));
	memset(sr, 0, sizeof(sr));
	
	cr = 0;
	
	msr = 0x40;
	
	fpscr = 0;
	
	externalInterruptPending = false;
	decrementerPending = false;
	iciPending = false;
	
	#if STATS
	dsiExceptions = 0;
	isiExceptions = 0;
	externalInterrupts = 0;
	decrementerInterrupts = 0;
	systemCalls = 0;
	#endif
	
	#if METRICS
	syscallIds.clear();
	#endif
	
	triggerException(SystemReset);
}

bool PPCCore::getCarry() {
	return sprs[XER] & CA;
}

void PPCCore::setCarry(bool carry) {
	if (carry) sprs[XER] |= CA;
	else {
		sprs[XER] &= ~CA;
	}
}

bool PPCCore::isMaskable(ExceptionType type) {
	return type == ExternalInterrupt || type == Decrementer || type == ICI;
}

void PPCCore::triggerException(ExceptionType type) {
	uint32_t vector = msr & 0x40 ? 0xFFF00000 : 0;
	
	if (!(msr & 0x8000) && isMaskable(type)) {
		if (type == ExternalInterrupt) externalInterruptPending = true;
		else if (type == Decrementer) decrementerPending = true;
		else if (type == ICI) iciPending = true;
	}
	else {
		#if STATS
		if (type == DSI) dsiExceptions++;
		else if (type == ISI) isiExceptions++;
		else if (type == ExternalInterrupt) externalInterrupts++;
		else if (type == Decrementer) decrementerInterrupts++;
		else if (type == SystemCall) systemCalls++;
		#endif
		
		#if METRICS
		if (type == SystemCall) {
			syscallIds[regs[0]]++;
		}
		#endif
		
		sprs[SRR0] = pc;
		sprs[SRR1] = (msr & 0xFF73) | 2;
		msr &= ~0x4EF70;
		
		if (type == DSI) {
			sprs[SRR0] -= 4;
		}
		
		pc = vector + 0x100 * type;
	}
}

void PPCCore::checkPendingExceptions() {
	if (msr & 0x8000) {
		if (externalInterruptPending) {
			externalInterruptPending = false;
			triggerException(ExternalInterrupt);
		}
		else if (decrementerPending) {
			decrementerPending = false;
			triggerException(Decrementer);
		}
		else if (iciPending) {
			iciPending = false;
			triggerException(ICI);
		}
	}
}
