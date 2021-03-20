
#include "cpu/arm/arminstruction.h"
#include "cpu/arm/armprocessor.h"
#include "common/exceptions.h"

static uint32_t getReg(ARMCore *core, int reg) {
	uint32_t value = core->regs[reg];
	if (reg == ARMCore::PC) {
		return value + 4;
	}
	return value;
}

template <class T>
void processLoadStore(ARMInstruction *instr, ARMProcessor *cpu, uint32_t offset, bool exchange) {
	uint32_t base = getReg(&cpu->core, instr->r0());
	uint32_t indexed = instr->u() ? base + offset : base - offset;
	uint32_t addr = instr->p() ? indexed : base;
	
	if (instr->l()) { // Load
		T value;
		if (!cpu->read<T>(addr, &value)) return;
		cpu->core.regs[instr->r1()] = value;
		
		if (instr->r1() == ARMCore::PC && exchange) {
			if (cpu->core.regs[ARMCore::PC] & 1) {
				cpu->core.setThumb(true);
				cpu->core.regs[ARMCore::PC] &= ~1;
			}
		}
	}
	else { // Store
		uint32_t value = getReg(&cpu->core, instr->r1());
		if (!cpu->write<T>(addr, value)) return;
	}
	
	if (instr->w() || !instr->p()) {
		cpu->core.regs[instr->r0()] = indexed;
	}
}

template <class T>
void processSwap(ARMInstruction *instr, ARMProcessor *cpu) {
	T temp;
	if (!cpu->read<T>(cpu->core.regs[instr->r0()], &temp)) return;
	if (!cpu->write<T>(cpu->core.regs[instr->r0()], cpu->core.regs[instr->r3()])) return;
	cpu->core.regs[instr->r1()] = temp;
}

void moveStatusRegister(ARMInstruction *instr, ARMProcessor *cpu, uint32_t value) {
	uint32_t mask = 0;
	if ((instr->value >> 19) & 1) mask |= 0xFF000000;
	if ((instr->value >> 18) & 1) mask |= 0x00FF0000;
	if ((instr->value >> 17) & 1) mask |= 0x0000FF00;
	if ((instr->value >> 16) & 1) mask |= 0x000000FF;
	
	if (instr->r()) {
		cpu->core.spsr = (cpu->core.spsr & ~mask) | (value & mask);
	}
	else {
		if (mask & 0xFF) {
			cpu->core.writeModeRegs();
		}
		cpu->core.cpsr = (cpu->core.cpsr & ~mask) | (value & mask);
		if (mask & 0xFF) {
			cpu->core.readModeRegs();
		}
	}
}

void ARMInstr_Multiply(ARMInstruction *instr, ARMProcessor *cpu) {
	if ((instr->value >> 23) & 1) { // Multiply long
		uint64_t result;
		if ((instr->value >> 22) & 1) { // Signed
			result = (int64_t)(int32_t)cpu->core.regs[instr->r2()] * (int32_t)cpu->core.regs[instr->r3()];
		}
		else {
			result = (uint64_t)cpu->core.regs[instr->r2()] * cpu->core.regs[instr->r3()];
		}
		
		if (instr->a()) {
			result += ((uint64_t)cpu->core.regs[instr->r0()] << 32) | cpu->core.regs[instr->r1()];
		}
		
		if (instr->s()) {
			cpu->core.cpsr.set(ARMCore::Z, result == 0);
			cpu->core.cpsr.set(ARMCore::N, result >> 63);
		}
		
		cpu->core.regs[instr->r0()] = result >> 32;
		cpu->core.regs[instr->r1()] = result & 0xFFFFFFFF;
	}
	else if ((instr->value >> 22) & 1) {
		cpu->core.triggerException(ARMCore::UndefinedInstruction);
	}
	else { // Multiply
		uint32_t result = cpu->core.regs[instr->r2()] * cpu->core.regs[instr->r3()];
		if (instr->a()) {
			result += cpu->core.regs[instr->r1()];
		}
		
		if (instr->s()) {
			cpu->core.cpsr.set(ARMCore::Z, result == 0);
			cpu->core.cpsr.set(ARMCore::N, result >> 31);
		}
		
		cpu->core.regs[instr->r0()] = result;
	}
}

void ARMInstr_CountLeadingZeros(ARMInstruction *instr, ARMProcessor *cpu) {
	uint32_t value = cpu->core.regs[instr->r3()];
	
	int bit;
	for (bit = 31; bit >= 0; bit--) {
		if (value & (1 << bit)) break;
	}
	
	cpu->core.regs[instr->r1()] = 31 - bit;
}

void ARMInstr_LoadStoreExtra(ARMInstruction *instr, ARMProcessor *cpu) {
	int offset;
	if ((instr->value >> 22) & 1) {
		offset = (instr->value & 0xF) | ((instr->value >> 4) & 0xF0);
	}
	else {
		offset = cpu->core.regs[instr->r3()];
	}
	
	int opcode = (instr->value >> 5) & 3;
	if (opcode == 1) processLoadStore<uint16_t>(instr, cpu, offset, false);
	else if (opcode == 2) {
		if (instr->l()) processLoadStore<int8_t>(instr, cpu, offset, false);
		else {
			int rd = instr->r1();
			if (rd % 2) {
				cpu->core.triggerException(ARMCore::UndefinedInstruction);
			}
			else {
				uint32_t base = getReg(&cpu->core, instr->r0());
				uint32_t indexed = instr->u() ? base + offset : base - offset;
				uint32_t addr = instr->p() ? indexed : base;
				if (!cpu->read<uint32_t>(addr, &cpu->core.regs[rd])) return;
				if (!cpu->read<uint32_t>(addr+4, &cpu->core.regs[rd+1])) return;
				if (instr->w() || !instr->p()) {
					cpu->core.regs[instr->r0()] = indexed;
				}
			}
		}
	}
	else if (opcode == 3) {
		if (instr->l()) processLoadStore<int16_t>(instr, cpu, offset, false);
		else {
			int rd = instr->r1();
			if (rd % 2) {
				cpu->core.triggerException(ARMCore::UndefinedInstruction);
			}
			else {
				uint32_t base = getReg(&cpu->core, instr->r0());
				uint32_t indexed = instr->u() ? base + offset : base - offset;
				uint32_t addr = instr->p() ? indexed : base;
				if (!cpu->write<uint32_t>(addr, getReg(&cpu->core, rd))) return;
				if (!cpu->write<uint32_t>(addr+4, getReg(&cpu->core, rd+1))) return;
				if (instr->w() || !instr->p()) {
					cpu->core.regs[instr->r0()] = indexed;
				}
			}
		}
	}
}

void ARMInstr_Swap(ARMInstruction *instr, ARMProcessor *cpu) {
	if (instr->b()) {
		processSwap<uint8_t>(instr, cpu);
	}
	else {
		processSwap<uint32_t>(instr, cpu);
	}
}

void ARMInstr_MoveStatusRegisterImm(ARMInstruction *instr, ARMProcessor *cpu) {
	uint32_t value = instr->value & 0xFF;
	int rot = instr->rotate() * 2;
	
	value = (value >> rot) | (value << (32 - rot));
	moveStatusRegister(instr, cpu, value);
}

void ARMInstr_MoveStatusRegisterReg(ARMInstruction *instr, ARMProcessor *cpu) {
	uint32_t value = getReg(&cpu->core, instr->r3());
	moveStatusRegister(instr, cpu, value);
}

void ARMInstr_CoprocessorTransfer(ARMInstruction *instr, ARMProcessor *cpu) {
	if (instr->l()) {
		uint32_t value;
		if (!cpu->coprocessorRead(instr->r2(), instr->cpopc(), &value, instr->r0(), instr->r3(), instr->cp())) {
			cpu->core.triggerException(ARMCore::UndefinedInstruction);
			return;
		}
		if (instr->r1() == ARMCore::PC) {
			cpu->core.cpsr = (cpu->core.cpsr & 0x0FFFFFFF) | (value & 0xF0000000);
		}
		else {
			cpu->core.regs[instr->r1()] = value;
		}
	}
	else {
		uint32_t value = cpu->core.regs[instr->r1()];
		if (!cpu->coprocessorWrite(instr->r2(), instr->cpopc(), value, instr->r0(), instr->r3(), instr->cp())) {
			cpu->core.triggerException(ARMCore::UndefinedInstruction);
		}
	}
}

void ARMInstruction::execute(ARMProcessor *cpu) {
	int type = (value >> 25) & 7;
	if (type == 0) {
		if ((value & 0x90) == 0x90) {
			if ((value & 0x60) == 0) {
				if ((value >> 24) & 1) {
					ARMInstr_Swap(this, cpu);
				}
				else {
					ARMInstr_Multiply(this, cpu);
				}
			}
			else {
				ARMInstr_LoadStoreExtra(this, cpu);
			}
		}
		else {
			//Miscellaneous instructions
			int type = (value >> 4) & 7;
			if (type == 0) {
				ARMInstr_MoveStatusRegisterReg(this, cpu);
			}
			else {
				ARMInstr_CountLeadingZeros(this, cpu);
			}
		}
	}
	else if (type == 1) ARMInstr_MoveStatusRegisterImm(this, cpu);
	else if (type == 7) ARMInstr_CoprocessorTransfer(this, cpu);
}
