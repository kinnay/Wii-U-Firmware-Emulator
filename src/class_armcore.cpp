
#include "class_armcore.h"
#include <string>

ARMCore::ARMCore() : mode(System), thumb(false) {}

void ARMCore::triggerException(ExceptionType type) {
	if (type == UndefinedInstruction) {
		regsUnd[1] = regs[PC];
		spsrUnd = cpsr;
		cpsr = (cpsr & ~0x1F) | 0x9B;
		setThumb(false);
		setMode(Undefined);
		regs[PC] = 0xFFFF0004;
	}
	else if (type == DataAbort) {
		regsAbt[1] = regs[PC] + 4;
		spsrAbt = cpsr;
		cpsr = (cpsr & ~0x1F) | 0x97;
		setThumb(false);
		setMode(Abort);
		regs[PC] = 0xFFFF0010;
	}
	else if (type == InterruptRequest) {
		if (!cpsr.get(I)) {
			regsIrq[1] = regs[PC] + 4;
			spsrIrq = cpsr;
			cpsr = (cpsr & ~0x1F) | 0x92;
			setThumb(false);
			setMode(IRQ);
			regs[PC] = 0xFFFF0018;
		}
	}
}

void ARMCore::setThumb(bool thumb) {
	this->thumb = thumb;
	cpsr.set(T, thumb);
}

void ARMCore::setMode(Mode mode) {
	writeModeRegs();
	this->mode = mode;
	readModeRegs();
}

void ARMCore::writeModeRegs() {
	if (this->mode == User || this->mode == System)
		memcpy(regsUser, regs, 15 * 4);
	else if (this->mode == FIQ) {
		memcpy(regsUser, regs, 8 * 4);
		memcpy(regsFiq, &regs[8], 7 * 4);
		spsrFiq = spsr;
	}
	else if (this->mode == IRQ) {
		memcpy(regsUser, regs, 12 * 4);
		memcpy(regsIrq, &regs[13], 2 * 4);
		spsrIrq = spsr;
	}
	else if (this->mode == SVC) {
		memcpy(regsUser, regs, 12 * 4);
		memcpy(regsSvc, &regs[13], 2 * 4);
		spsrSvc = spsr;
	}
	else if (this->mode == Abort) {
		memcpy(regsUser, regs, 12 * 4);
		memcpy(regsAbt, &regs[13], 2 * 4);
		spsrAbt = spsr;
	}
	else if (this->mode == Undefined) {
		memcpy(regsUser, regs, 12 * 4);
		memcpy(regsUnd, &regs[13], 2 * 4);
		spsrUnd = spsr;
	}
	regsUser[15] = regs[15];
}

void ARMCore::readModeRegs() {
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
