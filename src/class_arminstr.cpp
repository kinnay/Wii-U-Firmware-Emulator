
#include "class_arminstr.h"
#include "class_arminterp.h"
#include "errors.h"

inline uint32_t getShifted(ARMCore *core, int shift, int reg, bool update_cond) {
	uint32_t value = core->regs[reg];
	if (reg == ARMCore::PC) {
		if (shift & 1) value += 8;
		else value += 4;
	}
	
	int amount;
	if (shift & 1) {
		amount = core->regs[shift >> 4] & 0xFF;
		if (!amount) { //If this byte is zero, the value and carry will remain unchanged
			return value;
		}
	}
	else {
		//Need to handle instruction specified shift amount even if it's zero,
		//because LSR/ASR/ROR #0 => #32
		amount = shift >> 3;
	}
	
	//Shifts by more than 32 bits are undefined
	//Need to handle those separately
	switch ((shift >> 1) & 3) {
		case 0: //Logical left
			if (amount) {
				if (update_cond) {
					if (amount <= 32) core->cpsr.set(ARMCore::C, value & (1 << (32 - amount)));
					else core->cpsr.set(ARMCore::C, false);
				}
				
				if (amount < 32) value <<= amount;
				else value = 0;
			}
			break;
		case 1: //Logical right
			if (amount == 0) amount = 32;
			
			if (update_cond) {
				if (amount <= 32) core->cpsr.set(ARMCore::C, value & (1 << (amount - 1)));
				else core->cpsr.set(ARMCore::C, false);
			}
			
			if (amount < 32) value >>= amount;
			else value = 0;
			break;
		case 2: //Arithmetic right
			if (amount == 0) amount = 32;

			if (update_cond && amount) {
				if (amount <= 32) core->cpsr.set(ARMCore::C, value & (1 << (amount - 1)));
				else core->cpsr.set(ARMCore::C, value >> 31);
			}
			
			if (amount < 32) value = (int32_t)value >> amount;
			else value = 0;
			break;
		case 3: //Rotate right
			if (amount == 0) { //RRX
				bool carry = core->cpsr.get(ARMCore::C);
				if (update_cond) {
					core->cpsr.set(ARMCore::C, value & 1);
				}
				value = (value >> 1) | (carry << 31);
			}
			else {
				amount = (amount - 1) % 32 + 1; //Reduce into range 1 - 32
				
				if (amount != 32) {
					uint32_t rotmask = (1 << amount) - 1;
					value = (value >> amount) | ((value & rotmask) << (32 - amount));
				}	
				if (update_cond) {
					core->cpsr.set(ARMCore::C, value >> 31);
				}
			}
	}
	return value;
}

uint32_t ARM_Subtract(ARMCore *core, uint32_t v1, uint32_t v2, bool s) {
	uint32_t result = v1 - v2;
	if (s) {
		core->cpsr.set(ARMCore::C, v1 >= v2);
		if ((int32_t)v1 >= (int32_t)v2)
			core->cpsr.set(ARMCore::V, result >> 31);
		else core->cpsr.set(ARMCore::V, !(result >> 31));
	}	
	return result;
}

uint32_t ARM_Add(ARMCore *core, uint32_t v1, uint32_t v2, bool s) {
	uint32_t result = v1 + v2;
	if (s) {
		core->cpsr.set(ARMCore::C, v1 > 0xFFFFFFFF - v2);
		if ((int32_t)v1 >= -(int32_t)v2)
			core->cpsr.set(ARMCore::V, result >> 31);
		else core->cpsr.set(ARMCore::V, !(result >> 31));
	}
	return result;
}

uint32_t ARM_SubtractWithCarry(ARMCore *core, uint32_t v1, uint32_t v2, bool s) {
	bool carry = core->cpsr.get(ARMCore::C);
	uint32_t result = ARM_Subtract(core, v1, v2, s);
	if (!carry) {
		if (s) {
			if (result == 0) core->cpsr.set(ARMCore::C, true);
			else if (result == 0x80000000) core->cpsr.set(ARMCore::V, true);
		}
		result--;
	}
	return result;
}

bool ARMInstr_DataProcessing(ARMInstruction *instr, ARMInterpreter *cpu) {
	uint32_t opnd1 = cpu->core->regs[instr->r0()];
	if (instr->r0() == ARMCore::PC) {
		if (instr->shift() & 1) {
			opnd1 += 8;
		}
		else {
			opnd1 += 4;
		}
	}
	
	uint32_t opnd2;
	if (instr->i()) {
		int rot = instr->rotate() * 2;
		opnd2 = instr->value & 0xFF;
		if (rot) {
			uint32_t rotmask = (1 << rot) - 1;
			opnd2 = (opnd2 >> rot) | ((opnd2 & rotmask) << (32 - rot));
		}
	}
	else {
		opnd2 = getShifted(cpu->core, instr->shift(), instr->r3(), instr->s());
	}
	
	uint32_t result;
	int opcode = instr->opcode();
	if (opcode == 0 || opcode == 8) result = opnd1 & opnd2; //AND, TST
	else if (opcode == 1 || opcode == 9) result = opnd1 ^ opnd2;
	else if (opcode == 2 || opcode == 10) result = ARM_Subtract(cpu->core, opnd1, opnd2, instr->s()); //SUB, CMP
	else if (opcode == 3) result = ARM_Subtract(cpu->core, opnd2, opnd1, instr->s()); //RSB
	else if (opcode == 4 || opcode == 11) result = ARM_Add(cpu->core, opnd1, opnd2, instr->s()); //ADD, CMN
	else if (opcode == 5) { //ADC
		bool carry = cpu->core->cpsr.get(ARMCore::C);
		result = ARM_Add(cpu->core, opnd1, opnd2, instr->s());
		if (carry) {
			if (instr->s()) {
				if (result == 0xFFFFFFFF) cpu->core->cpsr.set(ARMCore::C, true);
				else if (result == 0x7FFFFFFF) cpu->core->cpsr.set(ARMCore::V, true);
			}
			result++;
		}
	}
	else if (opcode == 6) { result = ARM_SubtractWithCarry(cpu->core, opnd1, opnd2, instr->s()); } //SBC
	else if (opcode == 7) { result = ARM_SubtractWithCarry(cpu->core, opnd2, opnd1, instr->s()); } //RSC
	else if (opcode == 12) result = opnd1 | opnd2; //ORR
	else if (opcode == 13) result = opnd2; //MOV
	else if (opcode == 14) result = opnd1 & ~opnd2; //BIC
	else result = ~opnd2; //MVN
	
	if (opcode < 8 || opcode >= 12) {
		cpu->core->regs[instr->r1()] = result;
		if (instr->r1() == ARMCore::PC) {
			if (result & 1) {
				cpu->core->setThumb(true);
				cpu->core->regs[ARMCore::PC] &= ~1;
			}
		}
	}
	
	if (instr->s()) {
		if (instr->r1() == ARMCore::PC) {
			cpu->core->cpsr = cpu->core->spsr;
			cpu->core->setMode((ARMCore::Mode)(cpu->core->cpsr & 0x1F));
			cpu->core->setThumb(cpu->core->cpsr.get(ARMCore::T));
		}
		else {
			cpu->core->cpsr.set(ARMCore::Z, result == 0);
			cpu->core->cpsr.set(ARMCore::N, result >> 31);
		}
	}
		
	return true;
}

bool ARMInstr_MultiplyAccumulate(ARMInstruction *instr, ARMInterpreter *cpu) {
	uint32_t result = cpu->core->regs[instr->r2()] * cpu->core->regs[instr->r3()];
	if (instr->a())
		result += cpu->core->regs[instr->r1()];

	if (instr->s()) {
		cpu->core->cpsr.set(ARMCore::Z, result == 0);
		cpu->core->cpsr.set(ARMCore::N, result >> 31);
	}
	cpu->core->regs[instr->r0()] = result;
	return true;
}

bool ARMInstr_MultiplyAccumulateLong(ARMInstruction *instr, ARMInterpreter *cpu) {
	if (instr->a()) {
		NotImplementedError("ARM multiply long with accumulate 0x%08x", cpu->core->regs[15]);
		return false;
	}
	
	uint64_t result;
	if ((instr->value >> 22) & 1) {
		result = (int64_t)(int32_t)cpu->core->regs[instr->r2()] * (int32_t)cpu->core->regs[instr->r3()];
	}
	else {
		result = (uint64_t)cpu->core->regs[instr->r2()] * cpu->core->regs[instr->r3()];
	}
	
	if (instr->s()) {
		cpu->core->cpsr.set(ARMCore::Z, result == 0);
		cpu->core->cpsr.set(ARMCore::N, result >> 63);
	}
	cpu->core->regs[instr->r0()] = result >> 32;
	cpu->core->regs[instr->r1()] = result & 0xFFFFFFFF;
	return true;
}

bool ARMInstr_SingleDataTransfer(ARMInstruction *instr, ARMInterpreter *cpu) {
	uint32_t base = cpu->core->regs[instr->r0()];
	if (instr->r0() == ARMCore::PC) {
		base += 4;
	}
	
	uint32_t offset;
	if (instr->i()) {
		offset = getShifted(cpu->core, instr->shift(), instr->r3(), false);
	}
	else {
		offset = instr->value & 0xFFF;
	}
	
	uint32_t indexed = instr->u() ? base + offset : base - offset;
	uint32_t addr = instr->p() ? indexed : base;
	
	if (instr->l()) { //Load
		if (instr->b()) {
			uint8_t value;
			if (!cpu->read<uint8_t>(addr, &value)) return false;
			cpu->core->regs[instr->r1()] = value;
		}
		else {
			if (!cpu->read<uint32_t>(addr, &cpu->core->regs[instr->r1()])) return false;
			if (instr->r1() == ARMCore::PC) {
				if (cpu->core->regs[ARMCore::PC] & 1) {
					cpu->core->setThumb(true);
					cpu->core->regs[ARMCore::PC] &= ~1;
				}
			}
		}
	}
	else { //Store
		uint32_t value = cpu->core->regs[instr->r1()];
		if (instr->r1() == ARMCore::PC) {
			value += 8;
		}
		
		if (instr->b()) {
			if (!cpu->write<uint8_t>(addr, value)) return false;
		}
		else {
			if (!cpu->write<uint32_t>(addr, value)) return false;
		}
	}
	
	if (instr->w() || !instr->p()) {
		cpu->core->regs[instr->r0()] = indexed;
	}
	
	return true;
}

bool ARMInstr_LoadStoreHalfSigned(ARMInstruction *instr, ARMInterpreter *cpu) {
	uint32_t base = cpu->core->regs[instr->r0()];
	if (instr->r0() == ARMCore::PC) {
		base += 4;
	}
	
	uint32_t offset;
	if ((instr->value >> 22) & 1) {
		offset = (instr->value & 0xF) | ((instr->value >> 4) & 0xF0);
	}
	else {
		offset = cpu->core->regs[instr->r3()];
	}
	
	uint32_t indexed = instr->u() ? base + offset : base - offset;
	uint32_t addr = instr->p() ? indexed : base;
	
	if (instr->l()) { //Load
		if (instr->h()) {
			if ((instr->value >> 6) & 1) {
				int16_t value;
				if (!cpu->read<int16_t>(addr, &value)) return false;
				cpu->core->regs[instr->r1()] = value;
			}
			else {
				uint16_t value;
				if (!cpu->read<uint16_t>(addr, &value)) return false;
				cpu->core->regs[instr->r1()] = value;
			}
		}
		else {
			int8_t value;
			if (!cpu->read<int8_t>(addr, &value)) return false;
			cpu->core->regs[instr->r1()] = value;
		}
	}
	else { //Store
		uint32_t value = cpu->core->regs[instr->r1()];
		if (instr->r1() == ARMCore::PC) {
			value += 8;
		}
		
		if (instr->h()) {
			if (!cpu->write<uint16_t>(addr, value)) return false;
		}
		else {
			if (!cpu->write<uint8_t>(addr, value)) return false;
		}
	}
	
	if (instr->w() || !instr->p()) {
		cpu->core->regs[instr->r0()] = indexed;
	}
	
	return true;
}

bool ARMInstr_LoadStoreMultiple(ARMInstruction *instr, ARMInterpreter *cpu) {
	if (instr->value & (1 << 22) && instr->l() && instr->value & (1 << 15)) {
		NotImplementedError("Load/store multiple with S bit and PC at 0x%08x", cpu->core->regs[ARMCore::PC]);
		return false;
	}
	
	uint32_t addr = cpu->core->regs[instr->r0()];
	int adder = instr->u() ? 4 : -4;
	int reg = instr->u() ? 0 : 15;
	int inc = instr->u() ? 1 : -1;

	uint32_t *regs;
	if (instr->value & (1 << 22)) {
		cpu->core->writeModeRegs(); //Update regs
		regs = cpu->core->regsUser;
	}
	else {
		regs = cpu->core->regs;
	}

	for (int i = 0; i < 16; i++) {
		if (instr->value & (1 << reg)) {
			if (instr->p()) {
				addr += adder;
			}
			if (instr->l()) {
				if (!cpu->read<uint32_t>(addr, &regs[reg])) return false;
			}
			else {
				if (!cpu->write<uint32_t>(addr, regs[reg])) return false;
			}
			if (!instr->p()) {
				addr += adder;
			}
		}
		
		reg += inc;
	}
	
	if (cpu->core->regs[ARMCore::PC] & 1) {
		cpu->core->setThumb(true);
		cpu->core->regs[ARMCore::PC] &= ~1;
	}
	
	if (instr->value & (1 << 22)) {
		cpu->core->readModeRegs(); //Update regs
	}
	
	if (instr->w()) {
		cpu->core->regs[instr->r0()] = addr;
	}
	return true;
}

bool ARMInstr_Swap(ARMInstruction *instr, ARMInterpreter *cpu) {
	uint32_t memvalue;
	if (!cpu->read<uint32_t>(cpu->core->regs[instr->r0()], &memvalue)) return false;
	if (!cpu->write<uint32_t>(cpu->core->regs[instr->r0()], cpu->core->regs[instr->r3()])) return false;
	cpu->core->regs[instr->r1()] = memvalue;
	return true;
}

bool ARMInstr_Branch(ARMInstruction *instr, ARMInterpreter *cpu) {
	if (instr->link()) {
		cpu->core->regs[ARMCore::LR] = cpu->core->regs[ARMCore::PC];
	}
	cpu->core->regs[ARMCore::PC] += instr->offset() + 4;
	return true;
}

bool ARMInstr_BranchAndExchange(ARMInstruction *instr, ARMInterpreter *cpu) {
	if (instr->value & 0x20) {
		cpu->core->regs[ARMCore::LR] = cpu->core->regs[ARMCore::PC];
	}
	
	uint32_t dest = cpu->core->regs[instr->r3()];
	if (dest & 1) {
		cpu->core->setThumb(true);
		dest &= ~1;
	}
	cpu->core->regs[ARMCore::PC] = dest;
	return true;
}

bool ARMInstr_PSRTransfer(ARMInstruction *instr, ARMInterpreter *cpu) {
	if (!(instr->value & 0x200000)) { //MRS
		if (instr->r()) {
			cpu->core->regs[instr->r1()] = cpu->core->spsr;
		}
		else {
			cpu->core->regs[instr->r1()] = cpu->core->cpsr;
		}
	}

	else { //MSR
		uint32_t value;
		if (instr->i()) {
			int rot = instr->rotate() * 2;
			value = instr->value & 0xFF;
			if (rot) {
				uint32_t rotmask = (1 << rot) - 1;
				value = (value >> rot) | ((value & rotmask) << (32 - rot));
			}
		}
		else {
			value = cpu->core->regs[instr->r3()];
		}
		
		uint32_t mask = 0;
		if ((instr->value >> 19) & 1) mask |= 0xFF000000;
		if ((instr->value >> 18) & 1) mask |= 0x00FF0000;
		if ((instr->value >> 17) & 1) mask |= 0x0000FF00;
		if ((instr->value >> 16) & 1) mask |= 0x000000FF;
		
		if (instr->r()) {
			cpu->core->spsr = (cpu->core->spsr & ~mask) | (value & mask);
		}
		else {
			cpu->core->cpsr = (cpu->core->cpsr & ~mask) | (value & mask);
			if (mask & 0xFF) {
				cpu->core->setMode((ARMCore::Mode)(cpu->core->cpsr & 0x1F));
			}
		}
	}
	
	return true;
}

bool ARMInstr_CoprocessorRegisterTransfer(ARMInstruction *instr, ARMInterpreter *cpu) {
	if (instr->l()) {
		uint32_t value;
		if (!cpu->handleCoprocessorRead(instr->r2(), instr->cpopc(), &value, instr->r0(), instr->r3(), instr->cp())) {
			return false;
		}
		if (instr->r1() == ARMCore::PC) {
			cpu->core->cpsr = (cpu->core->cpsr & ~0xF0000000) | (value & 0xF0000000);
		}
		else {
			cpu->core->regs[instr->r1()] = value;
		}
		return true;
	}
	else {
		uint32_t value = cpu->core->regs[instr->r1()];
		if (instr->r1() == ARMCore::PC) value += 8;
		return cpu->handleCoprocessorWrite(instr->r2(), instr->cpopc(), value, instr->r0(), instr->r3(), instr->cp());
	}
}

bool ARMInstr_Undefined(ARMInstruction *instr, ARMInterpreter *cpu) {
	return cpu->handleUndefined();
}

bool ARMInstruction::execute(ARMInterpreter *cpu) {
	if ((value >> 28) == 0xF) {
		if ((value & 0xE000000) == 0xA000000) {
			//Branch and change to thumb
			NotImplementedError("Branch and change to thumb");
			return false;
		}
		return ARMInstr_Undefined(this, cpu);
	}
	
	if ((value & 0x01900000) != 0x01000000 && (
			(value & 0x0E000000) == 0x02000000 ||
			(value & 0x0E000090) == 0x00000010 ||
			!(value & 0x0E000010))
		) return ARMInstr_DataProcessing(this, cpu);
	
	if (!(value & 0x0E000000)) {
		if ((value & 0x90) != 0x90) {
			//Miscellaneous instructions
			if (!(value & 0xF0)) return ARMInstr_PSRTransfer(this, cpu);
			if ((value & 0xF0) == 0x10) {
				if (value & 0x400000) {
					NotImplementedError("Count leading zeros");
					return false;
				}
				return ARMInstr_BranchAndExchange(this, cpu);
			}
			if ((value & 0xF0) == 0x30) return ARMInstr_BranchAndExchange(this, cpu);
			if (value & 0x80) {
				NotImplementedError("Enhanced DSP multiplies");
				return false;
			}
			if ((value & 0x60) == 0x60) {
				NotImplementedError("Software breakpoint");
				return false;
			}
			NotImplementedError("Enhanced DSP add/subtracts");
			return false;
		}
		else {
			//Multiplies, extra load/stores
			if ((value & 0xD0) == 0xD0) {
				if (value & 0x100000) return ARMInstr_LoadStoreHalfSigned(this, cpu);
				if ((value & 0x500000) == 0x400000) {
					NotImplementedError("Load/store two words imm offset");
					return false;
				}
				else {
					NotImplementedError("Load/store two words reg offset");
					return false;
				}
			}
			else if (value & 0x20) return ARMInstr_LoadStoreHalfSigned(this, cpu);
			else {
				if ((value >> 24) & 1) return ARMInstr_Swap(this, cpu);
				if ((value >> 23) & 1) return ARMInstr_MultiplyAccumulateLong(this, cpu);
				return ARMInstr_MultiplyAccumulate(this, cpu);
			}
		}
	}
	
	if ((value & 0x0E000010) == 0x06000010 ||
		(value & 0x0FB00000) == 0x03000000)
		return ARMInstr_Undefined(this, cpu);
	if ((value & 0x0F000000) == 0x0F000000) {
		//Software interrupt
		NotImplementedError("Software interrupt");
		return false;
	}
	if ((value & 0x0F000000) == 0x0E000000) {
		if (value & 0x10) return ARMInstr_CoprocessorRegisterTransfer(this, cpu);
		else {
			//Coprocessor data processing
			NotImplementedError("Coprocessor data processing");
			return false;
		}
	}
	if ((value & 0x0E000000) == 0x0C000000) {
		//Coprocessor load/store
		NotImplementedError("Coprocessor load/store");
		return false;
	}
	if ((value & 0x0E000000) == 0x0A000000) return ARMInstr_Branch(this, cpu);
	if ((value & 0x0E000000) == 0x08000000) return ARMInstr_LoadStoreMultiple(this, cpu);
	if ((value & 0x0C000000) == 0x04000000) return ARMInstr_SingleDataTransfer(this, cpu);
	return ARMInstr_PSRTransfer(this, cpu);
}
