
#include "class_arminterp.h"
#include "class_armcore.h"
#include "class_arminstr.h"
#include "class_armthumb.h"
#include "class_endian.h"
#include "errors.h"

#include <algorithm>

ARMInterpreter::ARMInterpreter(ARMCore *core, IPhysicalMemory *physmem, IVirtualMemory *virtmem, bool bigEndian)
	: Interpreter(physmem, virtmem, bigEndian), core(core), coprocReadCB(0), coprocWriteCB(0) {}

bool ARMInterpreter::checkCondition(int type) {
	switch(type) {
		case EQ: return core->cpsr.get(ARMCore::Z);
		case NE: return !core->cpsr.get(ARMCore::Z);
		case CS: return core->cpsr.get(ARMCore::C);
		case CC: return !core->cpsr.get(ARMCore::C);
		case MI: return core->cpsr.get(ARMCore::N);
		case PL: return !core->cpsr.get(ARMCore::N);
		case VS: return core->cpsr.get(ARMCore::V);
		case VC: return !core->cpsr.get(ARMCore::V);
		case HI: return core->cpsr.get(ARMCore::C) && !core->cpsr.get(ARMCore::Z);
		case LS: return !core->cpsr.get(ARMCore::C) || core->cpsr.get(ARMCore::Z);
		case GE: return core->cpsr.get(ARMCore::N) == core->cpsr.get(ARMCore::V);
		case LT: return core->cpsr.get(ARMCore::N) != core->cpsr.get(ARMCore::V);
		case GT: return !core->cpsr.get(ARMCore::Z) && core->cpsr.get(ARMCore::N) == core->cpsr.get(ARMCore::V);
		case LE: return core->cpsr.get(ARMCore::Z) || core->cpsr.get(ARMCore::N) != core->cpsr.get(ARMCore::V);
		default: return true; //14, 15
	}
}

void ARMInterpreter::setCoprocReadCB(CoprocReadCB callback) { coprocReadCB = callback; }
void ARMInterpreter::setCoprocWriteCB(CoprocWriteCB callback) { coprocWriteCB = callback; }
void ARMInterpreter::setSoftwareInterruptCB(SoftwareInterruptCB callback) { softwareInterruptCB = callback; }
void ARMInterpreter::setUndefinedCB(UndefinedCB callback) { undefinedCB = callback; }

bool ARMInterpreter::handleCoprocessorRead(int coproc, int opc, uint32_t *value, int rn, int rm, int type) {
	if (!coprocReadCB) {
		RuntimeError("No coprocessor read callback installed");
		return false;
	}
	return coprocReadCB(coproc, opc, value, rn, rm, type);
}

bool ARMInterpreter::handleCoprocessorWrite(int coproc, int opc, uint32_t value, int rn, int rm, int type) {
	if (!coprocWriteCB) {
		RuntimeError("No coprocessor write callback installed");
		return false;
	}
	return coprocWriteCB(coproc, opc, value, rn, rm, type);
}

bool ARMInterpreter::handleSoftwareInterrupt(int value) {
	if (!softwareInterruptCB) {
		RuntimeError("No software interrupt callback installed");
		return false;
	}
	return softwareInterruptCB(value);
}

bool ARMInterpreter::handleUndefined() {
	if (!undefinedCB) {
		RuntimeError("No undefined instruction callback installed");
		return false;
	}
	return undefinedCB();
}

bool ARMInterpreter::stepARM() {
	ARMInstruction instr;
	if (!read<uint32_t>(core->regs[ARMCore::PC], &instr.value, true)) {
		return false;
	}

	core->regs[ARMCore::PC] += 4;
	
	if (!checkCondition(instr.cond()))
		return true;
	
	ARMInstrCallback func = instr.decode();
	if (func) {
		return func(this, instr);
	}
	return false;
}

bool ARMInterpreter::stepThumb() {
	ARMThumbInstr instr;
	if (!read<uint16_t>(core->regs[ARMCore::PC], &instr.value, true)) {
		return false;
	}
	
	core->regs[ARMCore::PC] += 2;
	
	ThumbInstrCallback func = instr.decode();
	if (func) {
		return func(this, instr);
	}
	return false;
}

bool ARMInterpreter::step() {
	if (core->thumb) {
		return stepThumb();
	}
	return stepARM();
}