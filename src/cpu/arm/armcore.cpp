
#include "common/logger.h"
#include "armcore.h"
#include <cstring>


ARMCore::ARMCore() {
	reset();
}


void ARMCore::reset() {
	memset(regs, 0, sizeof(regs));
	memset(regsUser, 0, sizeof(regsUser));
	memset(regsFiq, 0, sizeof(regsFiq));
	memset(regsIrq, 0, sizeof(regsIrq));
	memset(regsAbt, 0, sizeof(regsAbt));
	memset(regsUnd, 0, sizeof(regsUnd));
	
	control = 0x2000;
	domain = 0;
	ttbr = 0;
	dfsr = 0;
	ifsr = 0;
	far = 0;
	
	cpsr = 0;
	spsr = 0;
	spsrFiq = 0;
	spsrIrq = 0;
	spsrSvc = 0;
	spsrAbt = 0;
	spsrUnd = 0;
	
	triggerException(Reset);
}


void ARMCore::triggerException(ExceptionType type) {
	uint32_t vector = control & 0x2000 ? 0xFFFF0000 : 0;
	
	if (type == Reset) {
		setMode(SVC);
		cpsr.set(T, false);
		cpsr.set(F, true);
		cpsr.set(I, true);
		regs[PC] = vector;
	}
	else if (type == UndefinedInstruction) {
		regsUnd[1] = regs[PC];
		spsrUnd = cpsr;
		setMode(Undefined);
		cpsr.set(T, false);
		cpsr.set(I, true);
		regs[PC] = vector + 4;
	}
	else if (type == SoftwareInterrupt) {
		regsSvc[1] = regs[PC];
		spsrSvc = cpsr;
		setMode(SVC);
		cpsr.set(T, false);
		cpsr.set(I, true);
		regs[PC] = vector + 8;
	}
	else if (type == PrefetchAbort) {
		regsAbt[1] = regs[PC] + 4;
		spsrAbt = cpsr;
		setMode(Abort);
		cpsr.set(T, false);
		cpsr.set(I, true);
		regs[PC] = vector + 0xC;
	}
	else if (type == DataAbort) {
		regsAbt[1] = regs[PC] + 4;
		if (cpsr.get(T)) {
			regsAbt[1] += 2;
		}
		spsrAbt = cpsr;
		setMode(Abort);
		cpsr.set(T, false);
		cpsr.set(I, true);
		regs[PC] = vector + 0x10;
	}
	else if (type == InterruptRequest) {
		if (!cpsr.get(I)) {
			regsIrq[1] = regs[PC] + 4;
			spsrIrq = cpsr;
			setMode(IRQ);
			cpsr.set(T, false);
			cpsr.set(I, true);
			regs[PC] = vector + 0x18;
		}
	}
	else if (type == FastInterrupt) {
		if (!cpsr.get(F)) {
			regsFiq[1] = regs[PC] + 4;
			spsrFiq = cpsr;
			setMode(FIQ);
			cpsr.set(T, false);
			cpsr.set(F, true);
			cpsr.set(I, true);
			regs[PC] = vector + 0x1C;
		}
	}
}

bool ARMCore::isThumb() {
	return cpsr.get(T);
}

void ARMCore::setThumb(bool thumb) {
	cpsr.set(T, thumb);
}

ARMCore::ProcessorMode ARMCore::getMode() {
	return (ProcessorMode)cpsr.getfield(Mode);
}

void ARMCore::setMode(ProcessorMode mode) {
	writeModeRegs();
	cpsr.setfield(Mode, (uint32_t)mode);
	readModeRegs();
}

void ARMCore::writeModeRegs() {
	ProcessorMode mode = getMode();
	if (mode == User || mode == System){
		memcpy(regsUser, regs, 15 * 4);
	}
	else if (mode == FIQ) {
		memcpy(regsUser, regs, 8 * 4);
		memcpy(regsFiq, &regs[8], 7 * 4);
		spsrFiq = spsr;
	}
	else if (mode == IRQ) {
		memcpy(regsUser, regs, 12 * 4);
		memcpy(regsIrq, &regs[13], 2 * 4);
		spsrIrq = spsr;
	}
	else if (mode == SVC) {
		memcpy(regsUser, regs, 12 * 4);
		memcpy(regsSvc, &regs[13], 2 * 4);
		spsrSvc = spsr;
	}
	else if (mode == Abort) {
		memcpy(regsUser, regs, 12 * 4);
		memcpy(regsAbt, &regs[13], 2 * 4);
		spsrAbt = spsr;
	}
	else if (mode == Undefined) {
		memcpy(regsUser, regs, 12 * 4);
		memcpy(regsUnd, &regs[13], 2 * 4);
		spsrUnd = spsr;
	}
	regsUser[15] = regs[15];
}

void ARMCore::readModeRegs() {
	ProcessorMode mode = getMode();
	memcpy(regs, regsUser, 16 * 4);
	if (mode == FIQ) {
		memcpy(&regs[8], regsFiq, 7 * 4);
		spsr = spsrFiq;
	}
	else if (mode == IRQ) {
		memcpy(&regs[13], regsIrq, 2 * 4);
		spsr = spsrIrq;
	}
	else if (mode == SVC) {
		memcpy(&regs[13], regsSvc, 2 * 4);
		spsr = spsrSvc;
	}
	else if (mode == Abort) {
		memcpy(&regs[13], regsAbt, 2 * 4);
		spsr = spsrAbt;
	}
	else if (mode == Undefined) {
		memcpy(&regs[13], regsUnd, 2 * 4);
		spsr = spsrUnd;
	}
}
