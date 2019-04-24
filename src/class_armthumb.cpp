
#include "class_armthumb.h"
#include "class_arminterp.h"
#include "class_armcore.h"
#include "errors.h"

uint32_t AddWithFlags(ARMCore *core, uint32_t v1, uint32_t v2) {
	uint32_t result = v1 + v2;
	
	core->cpsr.set(ARMCore::Z, result == 0);
	core->cpsr.set(ARMCore::N, result >> 31);
	core->spsr.set(ARMCore::C, result < v1);
	if ((int32_t)v1 >= -(int32_t)v2)
		core->cpsr.set(ARMCore::V, result >> 31);
	else core->cpsr.set(ARMCore::V, !(result >> 31));
	
	return result;
}

uint32_t SubWithFlags(ARMCore *core, uint32_t v1, uint32_t v2) {
	uint32_t result = v1 - v2;

	core->cpsr.set(ARMCore::Z, result == 0);
	core->cpsr.set(ARMCore::N, result >> 31);
	core->cpsr.set(ARMCore::C, v1 >= v2);
	if ((int32_t)v1 >= (int32_t)v2)
		core->cpsr.set(ARMCore::V, result >> 31);
	else core->cpsr.set(ARMCore::V, !(result >> 31));
	
	return result;
}

uint32_t AddWithCarry(ARMCore *core, uint32_t v1, uint32_t v2) {
	bool carry = core->cpsr.get(ARMCore::C);
	uint32_t result = AddWithFlags(core, v1, v2);
	if (carry) {
		if (result == 0xFFFFFFFF) core->cpsr.set(ARMCore::C, true);
		else if (result == 0x7FFFFFFF) core->cpsr.set(ARMCore::V, true);
		result++;
	}
	return result;
}

uint32_t SubWithCarry(ARMCore *core, uint32_t v1, uint32_t v2) {
	bool carry = core->cpsr.get(ARMCore::C);
	uint32_t result = SubWithFlags(core, v1, v2);
	if (!carry) {
		if (result == 0) core->cpsr.set(ARMCore::C, true);
		else if (result == 0x80000000) core->cpsr.set(ARMCore::V, true);
		result--;
	}
	return result;
}

bool Thumb_AddSubtract(ARMThumbInstr *instr, ARMInterpreter *cpu) {
	int regin = (instr->value >> 3) & 7;
	int regout = instr->value & 7;
	
	uint32_t value = cpu->core->regs[regin];
	
	uint32_t imm = (instr->value >> 6) & 7;
	if (!instr->i()) {
		imm = cpu->core->regs[imm];
	}

	if (instr->value & 0x200) { //SUB
		cpu->core->regs[regout] = SubWithFlags(cpu->core, value, imm); 
	}
	else { //ADD
		cpu->core->regs[regout] = AddWithFlags(cpu->core, value, imm);
	}
	return true;
}

bool Thumb_AddSubCmpMovImm(ARMThumbInstr *instr, ARMInterpreter *cpu) {
	int reg = (instr->value >> 8) & 7;
	int imm = instr->value & 0xFF;
	
	int opcode = (instr->value >> 11) & 3;
	if (opcode == 0) { //MOV
		cpu->core->regs[reg] = imm;
		cpu->core->cpsr.set(ARMCore::Z, imm == 0);
		cpu->core->cpsr.set(ARMCore::N, imm >> 31);
	}
	else if (opcode == 1) { //CMP
		SubWithFlags(cpu->core, cpu->core->regs[reg], imm);
	}
	else if (opcode == 2) { //ADD
		cpu->core->regs[reg] = AddWithFlags(cpu->core, cpu->core->regs[reg], imm);
	}
	else { //SUB
		cpu->core->regs[reg] = SubWithFlags(cpu->core, cpu->core->regs[reg], imm);
	}
	return true;
}

bool Thumb_MoveShifted(ARMThumbInstr *instr, ARMInterpreter *cpu) {
	int imm = (instr->value >> 6) & 0x1F;
	int opcode = (instr->value >> 11) & 3;
	
	uint32_t value = cpu->core->regs[(instr->value >> 3) & 7];
	if (opcode == 0) value <<= imm; //LSL
	else if (opcode == 1) value >>= imm; //LSR
	else if (opcode == 2) value = ((int32_t)value) >> imm; //ASR
	
	cpu->core->regs[instr->value & 7] = value;
	cpu->core->cpsr.set(ARMCore::Z, value == 0);
	cpu->core->cpsr.set(ARMCore::N, value >> 31);
	return true;
}

bool Thumb_DataProcessing(ARMThumbInstr *instr, ARMInterpreter *cpu) {
	int opcode = (instr->value >> 6) & 0xF;
	int destreg = instr->value & 7;
	uint32_t destval = cpu->core->regs[destreg];
	uint32_t sourceval = cpu->core->regs[(instr->value >> 3) & 7];
	
	uint32_t result;
	if (opcode == 0 || opcode == 8) result = destval & sourceval; //AND, TST
	else if (opcode == 1) result = destval ^ sourceval; //EOR
	else if (opcode == 2) result = destval << sourceval; //LSL
	else if (opcode == 3) result = destval >> sourceval; //LSR
	else if (opcode == 4) result = (int32_t)destval >> sourceval; //ASR
	else if (opcode == 5) result = AddWithCarry(cpu->core, destval, sourceval); //ADC
	else if (opcode == 6) result = SubWithCarry(cpu->core, destval, sourceval); //SBC
	else if (opcode == 7) { //ROR
		uint32_t rotmask = (1 << sourceval) - 1;
		result = (destval >> sourceval) | ((destval & rotmask) << (32 - sourceval));
	}
	else if (opcode == 9) result = -(int32_t)sourceval; //NEG
	else if (opcode == 10) result = SubWithFlags(cpu->core, destval, sourceval); //CMP
	else if (opcode == 12) result = destval | sourceval; //ORR
	else if (opcode == 13) result = destval * sourceval; //MUL
	else if (opcode == 14) result = destval & ~sourceval; //BIC
	else if (opcode == 15) result = ~sourceval; //MVN
	else {
		NotImplementedError("Thumb data processing opcode %i", opcode);
		return false;
	}
	
	if (opcode != 8 && opcode != 10 && opcode != 11) {
		cpu->core->regs[destreg] = result;
	}
	
	cpu->core->cpsr.set(ARMCore::Z, result == 0);
	cpu->core->cpsr.set(ARMCore::N, result >> 31);
	return true;
}

bool Thumb_SpecialDataProcessing(ARMThumbInstr *instr, ARMInterpreter *cpu) {
	int reg1 = (instr->value & 7) + ((instr->value >> 4) & 8);
	int reg2 = (instr->value >> 3) & 0xF;
	
	int opcode = (instr->value >> 8) & 3;
	if (opcode == 0) { //ADD
		cpu->core->regs[reg1] += cpu->core->regs[reg2];
	}
	else if (opcode == 1) { //CMP
		SubWithFlags(cpu->core, cpu->core->regs[reg1], cpu->core->regs[reg2]);
	}
	else if (opcode == 2) { //MOV
		cpu->core->regs[reg1] = cpu->core->regs[reg2];
	}
	return true;
}

bool Thumb_AddToSPOrPC(ARMThumbInstr *instr, ARMInterpreter *cpu) {
	uint32_t value = instr->value & (1 << 11) ?
		cpu->core->regs[ARMCore::SP] : cpu->core->regs[ARMCore::PC] + 2;
	cpu->core->regs[(instr->value >> 8) & 7] = value + (instr->value & 0xFF) * 4;
	return true;
}

bool Thumb_AdjustStackPointer(ARMThumbInstr *instr, ARMInterpreter *cpu) {
	int offset = instr->value & 0x7F;
	if (instr->value & 0x80) {
		cpu->core->regs[ARMCore::SP] -= offset * 4;
	}
	else {
		cpu->core->regs[ARMCore::SP] += offset * 4;
	}
	return true;
}

bool Thumb_PushPopRegisterList(ARMThumbInstr *instr, ARMInterpreter *cpu) {
	uint32_t addr = cpu->core->regs[ARMCore::SP];

	if (instr->l()) {
		for (int i = 0; i < 8; i++) {
			if (instr->value & (1 << i)) {
				if (!cpu->read<uint32_t>(addr, &cpu->core->regs[i])) return false;
				addr += 4;
			}
		}
		if (instr->r()) {
			if (!cpu->read<uint32_t>(addr, &cpu->core->regs[ARMCore::PC])) return false;
			if (!(cpu->core->regs[ARMCore::PC] & 1)) {
				cpu->core->setThumb(false);
			}
			else {
				cpu->core->regs[ARMCore::PC] &= ~1;
			}
			addr += 4;
		}
	}
	
	else {
		if (instr->r()) {
			addr -= 4;
			if (!cpu->write<uint32_t>(addr, cpu->core->regs[ARMCore::LR])) return false;
		}
		for (int i = 7; i >= 0; i--) {
			if (instr->value & (1 << i)) {
				addr -= 4;
				if (!cpu->write<uint32_t>(addr, cpu->core->regs[i])) return false;
			}
		}
	}
	
	cpu->core->regs[ARMCore::SP] = addr;
	return true;
}

bool Thumb_LoadStoreStack(ARMThumbInstr *instr, ARMInterpreter *cpu) {
	int reg = (instr->value >> 8) & 7;
	uint32_t addr = cpu->core->regs[ARMCore::SP] + (instr->value & 0xFF) * 4;
	if (instr->l()) {
		return cpu->read<uint32_t>(addr, &cpu->core->regs[reg]);
	}
	return cpu->write<uint32_t>(addr, cpu->core->regs[reg]);
}

bool Thumb_LoadPCRelative(ARMThumbInstr *instr, ARMInterpreter *cpu) {
	uint32_t addr = ((cpu->core->regs[ARMCore::PC] + 2) & ~3) + (instr->value & 0xFF) * 4;
	return cpu->read<uint32_t>(addr, &cpu->core->regs[(instr->value >> 8) & 7]);
}

bool Thumb_LoadStoreImmOffs(ARMThumbInstr *instr, ARMInterpreter *cpu) {
	int reg = instr->value & 7;
	uint32_t addr = cpu->core->regs[(instr->value >> 3) & 7];
	uint32_t offset = (instr->value >> 6) & 0x1F;
	
	if (instr->b()) {
		addr += offset;
		if (instr->l()) {
			uint8_t value;
			if (!cpu->read<uint8_t>(addr, &value)) return false;
			cpu->core->regs[reg] = value;
			return true;
		}
		return cpu->write<uint8_t>(addr, cpu->core->regs[reg]);
	}
	
	addr += offset * 4;
	if (instr->l()) {
		return cpu->read<uint32_t>(addr, &cpu->core->regs[reg]);
	}
	return cpu->write<uint32_t>(addr, cpu->core->regs[reg]);
}

bool Thumb_LoadStoreRegOffs(ARMThumbInstr *instr, ARMInterpreter *cpu) {
	int reg = instr->value & 7;
	uint32_t regval = cpu->core->regs[reg];
	uint32_t addr = cpu->core->regs[(instr->value >> 3) & 7];
	addr += cpu->core->regs[(instr->value >> 6) & 7];
	
	int opcode = (instr->value >> 9) & 7;
	if (opcode == 0) return cpu->write<uint32_t>(addr, regval);
	if (opcode == 1) return cpu->write<uint16_t>(addr, regval);
	if (opcode == 2) return cpu->write<uint8_t>(addr, regval);
	if (opcode == 3) {
		int8_t value;
		if (!cpu->read<int8_t>(addr, &value)) return false;
		cpu->core->regs[reg] = value;
		return true;
	}
	if (opcode == 4) return cpu->read<uint32_t>(addr, &cpu->core->regs[reg]);
	if (opcode == 5) {
		uint16_t value;
		if (!cpu->read<uint16_t>(addr, &value)) return false;
		cpu->core->regs[reg] = value;
		return true;
	}
	if (opcode == 6) {
		uint8_t value;
		if (!cpu->read<uint8_t>(addr, &value)) return false;
		cpu->core->regs[reg] = value;
		return true;
	}
	
	int16_t value;
	if (!cpu->read<int16_t>(addr, &value)) return false;
	cpu->core->regs[reg] = value;
	return true;
}

bool Thumb_LoadStoreHalf(ARMThumbInstr *instr, ARMInterpreter *cpu) {
	uint32_t addr = cpu->core->regs[(instr->value >> 3) & 7];
	addr += (instr->value >> 5) & 0x3E;
	
	if (instr->l()) {
		uint16_t value;
		if (!cpu->read<uint16_t>(addr, &value)) return false;
		cpu->core->regs[instr->value & 7] = value;
		return true;
	}
	return cpu->write<uint16_t>(addr, cpu->core->regs[instr->value & 7]);
}

bool Thumb_LoadStoreMultiple(ARMThumbInstr *instr, ARMInterpreter *cpu) {
	int reg = (instr->value >> 8) & 7;
	uint32_t addr = cpu->core->regs[reg];
	for (int i = 0; i < 8; i++) {
		if (instr->value & (1 << i)) {
			if (instr->l()) {
				if (!cpu->read<uint32_t>(addr, &cpu->core->regs[i])) return false;
				addr += 4;
			}
			else {
				if (!cpu->write<uint32_t>(addr, cpu->core->regs[i])) return false;
				addr += 4;
			}
		}
	}
	cpu->core->regs[reg] = addr;
	return true;
}

bool Thumb_BranchExchange(ARMThumbInstr *instr, ARMInterpreter *cpu) {
	int reg = (instr->value >> 3) & 0xF;
	uint32_t dest = cpu->core->regs[reg];
	if (reg == ARMCore::PC) dest += 2;
	
	if ((instr->value >> 7) & 1) {
		cpu->core->regs[ARMCore::LR] = cpu->core->regs[ARMCore::PC] | 1;
	}

	if (!(dest & 1)) {
		cpu->core->setThumb(false);
	}
	else {
		dest &= ~1;
	}
	cpu->core->regs[ARMCore::PC] = dest;
	return true;
}

bool Thumb_UnconditionalBranch(ARMThumbInstr *instr, ARMInterpreter *cpu) {
	int offset = instr->value & 0x7FF;
	if (offset & 0x400) offset -= 0x800;
	cpu->core->regs[ARMCore::PC] += offset * 2 + 2;
	return true;
}

bool Thumb_ConditionalBranch(ARMThumbInstr *instr, ARMInterpreter *cpu) {
	if (cpu->checkCondition((instr->value >> 8) & 0xF)) {
		int offset = instr->value & 0xFF;
		if (offset & 0x80) offset -= 0x100;
		cpu->core->regs[ARMCore::PC] += offset * 2 + 2;
	}
	return true;
}

bool Thumb_LongBranchWithLink(ARMThumbInstr *instr, ARMInterpreter *cpu) {
	uint32_t offset = instr->value & 0x7FF;
	if (instr->h()) { //Instruction 2
		uint32_t target = cpu->core->regs[ARMCore::LR] + (offset << 1);
		cpu->core->regs[ARMCore::LR] = cpu->core->regs[ARMCore::PC] | 1;
		cpu->core->regs[ARMCore::PC] = target;
	}
	else { //Instruction 1
		if (offset & 0x400) offset -= 0x800;
		cpu->core->regs[ARMCore::LR] = cpu->core->regs[ARMCore::PC] + 2 + (offset << 12);
	}
	return true;
}

bool Thumb_SoftwareInterrupt(ARMThumbInstr *instr, ARMInterpreter *cpu) {
	return cpu->handleSoftwareInterrupt(instr->value & 0xFF);
}

bool ARMThumbInstr::execute(ARMInterpreter *cpu) {
	if (!(value >> 13)) {
		if (((value >> 11) & 3) == 3) return Thumb_AddSubtract(this, cpu);
		return Thumb_MoveShifted(this, cpu);
	}
	if ((value >> 13) == 1) return Thumb_AddSubCmpMovImm(this, cpu);
	if ((value >> 10) == 0x10) return Thumb_DataProcessing(this, cpu);
	if ((value >> 8) == 0x47) return Thumb_BranchExchange(this, cpu);
	if ((value >> 10) == 0x11) return Thumb_SpecialDataProcessing(this, cpu);
	if ((value >> 11) == 9) return Thumb_LoadPCRelative(this, cpu);
	if ((value >> 12) == 5) return Thumb_LoadStoreRegOffs(this, cpu);
	if ((value >> 13) == 3) return Thumb_LoadStoreImmOffs(this, cpu);
	if ((value >> 12) == 8) return Thumb_LoadStoreHalf(this, cpu);
	if ((value >> 12) == 9) return Thumb_LoadStoreStack(this, cpu);
	if ((value >> 12) == 10) return Thumb_AddToSPOrPC(this, cpu);
	if ((value >> 12) == 11) {
		if (((value >> 8) & 0xF) == 0) return Thumb_AdjustStackPointer(this, cpu);
		if (((value >> 8) & 0xF) == 0xE) {
			NotImplementedError("Thumb software breakpoint");
			return false;
		}
		return Thumb_PushPopRegisterList(this, cpu);
	}
	if ((value >> 12) == 12) return Thumb_LoadStoreMultiple(this, cpu);
	if ((value >> 12) == 13) {
		if (((value >> 8) & 0xF) == 14) {
			NotImplementedError("Thumb undefined instruction");
			return false;
		}
		if (((value >> 8) & 0xF) == 15) return Thumb_SoftwareInterrupt(this, cpu);
		return Thumb_ConditionalBranch(this, cpu);
	}
	if ((value >> 11) == 0x1C) return Thumb_UnconditionalBranch(this, cpu);
	if ((value >> 11) == 0x1D) {
		if (value & 1) {
			NotImplementedError("Thumb undefined instruction");
			return false;
		}
		NotImplementedError("Thumb BLX suffix");
		return false;
	}
	return Thumb_LongBranchWithLink(this, cpu);
}
