
#include "armthumbinstr.h"
#include "armprocessor.h"

void Thumb_LoadStoreRegOffs(ARMThumbInstr *instr, ARMProcessor *cpu) {
	int reg = instr->value & 7;
	uint32_t regval = cpu->core.regs[reg];
	uint32_t addr = cpu->core.regs[(instr->value >> 3) & 7];
	addr += cpu->core.regs[(instr->value >> 6) & 7];
	
	int opcode = (instr->value >> 9) & 7;
	if (opcode == 0) cpu->write<uint32_t>(addr, regval);
	else if (opcode == 1) cpu->write<uint16_t>(addr, regval);
	else if (opcode == 2) cpu->write<uint8_t>(addr, regval);
	else if (opcode == 3) {
		int8_t value;
		if (cpu->read<int8_t>(addr, &value)) {
			cpu->core.regs[reg] = value;
		}
	}
	else if (opcode == 4) cpu->read<uint32_t>(addr, &cpu->core.regs[reg]);
	else if (opcode == 5) {
		uint16_t value;
		if (cpu->read<uint16_t>(addr, &value)) {
			cpu->core.regs[reg] = value;
		}
	}
	else if (opcode == 6) {
		uint8_t value;
		if (cpu->read<uint8_t>(addr, &value)) {
			cpu->core.regs[reg] = value;
		}
	}
	else {
		int16_t value;
		if (cpu->read<int16_t>(addr, &value)) {
			cpu->core.regs[reg] = value;
		}
	}
}

void ARMThumbInstr::execute(ARMProcessor *cpu) {
	Thumb_LoadStoreRegOffs(this, cpu);
}
