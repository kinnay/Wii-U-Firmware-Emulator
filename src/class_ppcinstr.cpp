
#include "class_ppcinstr.h"
#include "class_ppcinterp.h"
#include "class_ppccore.h"
#include "errors.h"

/********** HELPER FUNCTIONS **********/

uint32_t rotl(uint32_t value, int bits) {
	if (!bits) return value;
	return (value << bits) | (value >> (32 - bits));
}

uint32_t genmask(int start, int end) {
	if (start <= end) {
		return (0xFFFFFFFF >> (31 - (end - start))) << (31 - end);
	}
	return (0xFFFFFFFF >> start) | (0xFFFFFFFF << (31 - end));
}

int cntlzw(uint32_t value) {
	if (!value) return 32;
	
	int zeroes = 0;
	while (!(value & 0x80000000)) {
		zeroes++;
		value <<= 1;
	}
	return zeroes;
}

bool PPC_CheckCondition(PPCInterpreter *cpu, PPCInstruction instr) {
	int bo = instr.bo();
	if (!(bo & 4)) {
		cpu->core->ctr--;
		if ((bo & 2) && cpu->core->ctr != 0) return false;
		else if (cpu->core->ctr == 0) return false;
	}
	if (bo & 0x10) return true;
	if (bo & 8) return (cpu->core->cr >> (31 - instr.bi())) & 1;
	return !((cpu->core->cr >> (31 - instr.bi())) & 1);
}

void PPC_UpdateConditions(PPCCore *core, uint32_t result) {
	core->cr.set(PPCCore::LT, result >> 31);
	core->cr.set(PPCCore::GT, !(result >> 31) && result != 0);
	core->cr.set(PPCCore::EQ, result == 0);
}

/********** NORMAL INSTRUCTIONS **********/

bool PPCInstr_addi(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t source = instr.rA() ? cpu->core->regs[instr.rA()] : 0;
	cpu->core->regs[instr.rD()] = source + instr.simm();
	return true;
}

bool PPCInstr_addis(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t source = instr.rA() ? cpu->core->regs[instr.rA()] : 0;
	cpu->core->regs[instr.rD()] = source + (instr.simm() << 16);
	return true;
}

bool PPCInstr_mulli(PPCInterpreter *cpu, PPCInstruction instr) {
	cpu->core->regs[instr.rD()] = cpu->core->regs[instr.rA()] * instr.simm();
	return true;
}

bool PPCInstr_ori(PPCInterpreter *cpu, PPCInstruction instr) {
	cpu->core->regs[instr.rA()] = cpu->core->regs[instr.rS()] | instr.uimm();
	return true;
}

bool PPCInstr_oris(PPCInterpreter *cpu, PPCInstruction instr) {
	cpu->core->regs[instr.rA()] = cpu->core->regs[instr.rS()] | (instr.uimm() << 16);
	return true;
}

bool PPCInstr_andi(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t result = cpu->core->regs[instr.rS()] & instr.uimm();
	PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr.rA()] = result;
	return true;
}

bool PPCInstr_andis(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t result = cpu->core->regs[instr.rS()] & (instr.uimm() << 16);
	PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr.rA()] = result;
	return true;
}

bool PPCInstr_xori(PPCInterpreter *cpu, PPCInstruction instr) {
	cpu->core->regs[instr.rA()] = cpu->core->regs[instr.rS()] ^ instr.uimm();
	return true;
}

bool PPCInstr_xoris(PPCInterpreter *cpu, PPCInstruction instr) {
	cpu->core->regs[instr.rA()] = cpu->core->regs[instr.rS()] ^ (instr.uimm() << 16);
	return true;
}

bool PPCInstr_addic(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t left = cpu->core->regs[instr.rA()];
	uint32_t right = instr.simm();
	uint64_t result = (uint64_t)left + right;
	cpu->core->xer.set(PPCCore::CA, result >> 32);
	cpu->core->regs[instr.rD()] = (uint32_t)result;
	return true;
}

bool PPCInstr_addic_rc(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t left = cpu->core->regs[instr.rA()];
	uint32_t right = instr.simm();
	uint64_t result = (uint64_t)left + right;
	cpu->core->xer.set(PPCCore::CA, result >> 32);
	PPC_UpdateConditions(cpu->core, (uint32_t)result);
	cpu->core->regs[instr.rD()] = (uint32_t)result;
	return true;
}

bool PPCInstr_subfic(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t left = cpu->core->regs[instr.rA()];
	uint32_t right = instr.simm();
	uint64_t result = (uint64_t)~left + right + 1;
	cpu->core->xer.set(PPCCore::CA, result >> 32);
	cpu->core->regs[instr.rD()] = (uint32_t)result;
	return true;
}

bool PPCInstr_srawi(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t value = cpu->core->regs[instr.rS()];
	uint32_t result = (int32_t)value >> instr.sh();

	if (instr.sh() && (value >> 31)) {
		cpu->core->xer.set(PPCCore::CA, value & genmask(32 - instr.sh(), 31));
	}
	else {
		cpu->core->xer.set(PPCCore::CA, 0);
	}

	if (instr.rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr.rA()] = result;
	return true;
}

bool PPCInstr_add(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t result = cpu->core->regs[instr.rA()] + cpu->core->regs[instr.rB()];
	if (instr.rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr.rD()] = result;
	return true;
}

bool PPCInstr_addc(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t left = cpu->core->regs[instr.rA()];
	uint32_t right = cpu->core->regs[instr.rB()];
	uint64_t result = (uint64_t)left + right;
	cpu->core->xer.set(PPCCore::CA, result >> 32);
	if (instr.rc()) PPC_UpdateConditions(cpu->core, (uint32_t)result);
	cpu->core->regs[instr.rD()] = (uint32_t)result;
	return true;
}

bool PPCInstr_adde(PPCInterpreter *cpu, PPCInstruction instr) {
	uint64_t result = (uint64_t)cpu->core->regs[instr.rA()] + cpu->core->regs[instr.rB()] + cpu->core->xer.get(PPCCore::CA);
	cpu->core->xer.set(PPCCore::CA, result >> 32);
	if (instr.rc()) PPC_UpdateConditions(cpu->core, (uint32_t)result);
	cpu->core->regs[instr.rD()] = (uint32_t)result;
	return true;
}

bool PPCInstr_addze(PPCInterpreter *cpu, PPCInstruction instr) {
	uint64_t result = (uint64_t)cpu->core->regs[instr.rA()] + cpu->core->xer.get(PPCCore::CA);
	cpu->core->xer.set(PPCCore::CA, result >> 32);
	if (instr.rc()) PPC_UpdateConditions(cpu->core, (uint32_t)result);
	cpu->core->regs[instr.rD()] = (uint32_t)result;
	return true;
}

bool PPCInstr_subf(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t result = cpu->core->regs[instr.rB()] - cpu->core->regs[instr.rA()];
	if (instr.rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr.rD()] = result;
	return true;
}

bool PPCInstr_subfc(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t left = cpu->core->regs[instr.rA()];
	uint32_t right = cpu->core->regs[instr.rB()];
	uint64_t result = (uint64_t)~left + right + 1;
	cpu->core->xer.set(PPCCore::CA, result >> 32);
	if (instr.rc()) PPC_UpdateConditions(cpu->core, (uint32_t)result);
	cpu->core->regs[instr.rD()] = (uint32_t)result;
	return true;
}

bool PPCInstr_subfe(PPCInterpreter *cpu, PPCInstruction instr) {
	uint64_t result = (uint64_t)~cpu->core->regs[instr.rA()] + cpu->core->regs[instr.rB()] + cpu->core->xer.get(PPCCore::CA);
	cpu->core->xer.set(PPCCore::CA, result >> 32);
	if (instr.rc()) PPC_UpdateConditions(cpu->core, (uint32_t)result);
	cpu->core->regs[instr.rD()] = (uint32_t)result;
	return true;
}

bool PPCInstr_subfze(PPCInterpreter *cpu, PPCInstruction instr) {
	uint64_t result = (uint64_t)~cpu->core->regs[instr.rA()] + cpu->core->xer.get(PPCCore::CA);
	cpu->core->xer.set(PPCCore::CA, result >> 32);
	if (instr.rc()) PPC_UpdateConditions(cpu->core, (uint32_t)result);
	cpu->core->regs[instr.rD()] = (uint32_t)result;
	return true;
}

bool PPCInstr_mullw(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t result = cpu->core->regs[instr.rA()] * cpu->core->regs[instr.rB()];
	if (instr.rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr.rD()] = result;
	return true;
}

bool PPCInstr_mulhw(PPCInterpreter *cpu, PPCInstruction instr) {
	uint64_t result = (int64_t)cpu->core->regs[instr.rA()] * (int64_t)cpu->core->regs[instr.rB()];
	if (instr.rc()) PPC_UpdateConditions(cpu->core, result >> 32);
	cpu->core->regs[instr.rD()] = result >> 32;
	return true;
}

bool PPCInstr_mulhwu(PPCInterpreter *cpu, PPCInstruction instr) {
	uint64_t result = (uint64_t)cpu->core->regs[instr.rA()] * cpu->core->regs[instr.rB()];
	if (instr.rc()) PPC_UpdateConditions(cpu->core, result >> 32);
	cpu->core->regs[instr.rD()] = result >> 32;
	return true;
}

bool PPCInstr_divwu(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t dividend = cpu->core->regs[instr.rA()];
	uint32_t divisor = cpu->core->regs[instr.rB()];
	if (divisor != 0) {
		uint32_t result = dividend / divisor;
		if (instr.rc()) PPC_UpdateConditions(cpu->core, result);
		cpu->core->regs[instr.rD()] = result;
	}
	return true;
}

bool PPCInstr_divw(PPCInterpreter *cpu, PPCInstruction instr) {
	int32_t dividend = cpu->core->regs[instr.rA()];
	int32_t divisor = cpu->core->regs[instr.rB()];
	if (divisor != 0) {
		int32_t result = dividend / divisor;
		if (instr.rc()) PPC_UpdateConditions(cpu->core, result);
		cpu->core->regs[instr.rD()] = result;
	}
	return true;
}

bool PPCInstr_or(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t result = cpu->core->regs[instr.rS()] | cpu->core->regs[instr.rB()];
	if (instr.rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr.rA()] = result;
	return true;
}

bool PPCInstr_and(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t result = cpu->core->regs[instr.rS()] & cpu->core->regs[instr.rB()];
	if (instr.rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr.rA()] = result;
	return true;
}

bool PPCInstr_xor(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t result = cpu->core->regs[instr.rS()] ^ cpu->core->regs[instr.rB()];
	if (instr.rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr.rA()] = result;
	return true;
}

bool PPCInstr_nor(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t result = ~(cpu->core->regs[instr.rS()] | cpu->core->regs[instr.rB()]);
	if (instr.rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr.rA()] = result;
	return true;
}

bool PPCInstr_andc(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t result = cpu->core->regs[instr.rS()] & ~cpu->core->regs[instr.rB()];
	if (instr.rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr.rA()] = result;
	return true;
}

bool PPCInstr_slw(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t bits = cpu->core->regs[instr.rB()] & 0x3F;
	uint32_t result = 0;
	if (!(bits & 0x20)) {
		result = cpu->core->regs[instr.rS()] << bits;
	}
	if (instr.rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr.rA()] = result;
	return true;
}

bool PPCInstr_srw(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t bits = cpu->core->regs[instr.rB()] & 0x3F;
	uint32_t result = 0;
	if (!(bits & 0x20)) {
		result = cpu->core->regs[instr.rS()] >> bits;
	}
	if (instr.rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr.rA()] = result;
	return true;
}

bool PPCInstr_sraw(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t value = cpu->core->regs[instr.rS()];
	uint32_t bits = cpu->core->regs[instr.rB()] & 0x3F;
	
	uint32_t result;
	if (!(bits & 0x20)) {
		result = (int32_t)value >> bits;
		if (bits && (value >> 31)) {
			cpu->core->xer.set(PPCCore::CA, value & genmask(32 - bits, 31));
		}
		else {
			cpu->core->xer.set(PPCCore::CA, 0);
		}
	}
	else {
		result = (int32_t)value >> 31;
		cpu->core->xer.set(PPCCore::CA, value >> 31);
	}
	
	if (instr.rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr.rA()] = result;
	return true;
}

bool PPCInstr_rlwinm(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t result = rotl(cpu->core->regs[instr.rS()], instr.sh()) & genmask(instr.mb(), instr.me());
	if (instr.rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr.rA()] = result;
	return true;
}

bool PPCInstr_rlwimi(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t mask = genmask(instr.mb(), instr.me());
	uint32_t temp = rotl(cpu->core->regs[instr.rS()], instr.sh()) & mask;
	uint32_t result = (cpu->core->regs[instr.rA()] & ~mask) | temp;
	if (instr.rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr.rA()] = result;
	return true;
}

bool PPCInstr_neg(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t result = -(int32_t)cpu->core->regs[instr.rA()];
	if (instr.rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr.rD()] = result;
	return true;
}

bool PPCInstr_extsb(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t result = (int8_t)cpu->core->regs[instr.rS()];
	if (instr.rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr.rA()] = result;
	return true;
}

bool PPCInstr_extsh(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t result = (int16_t)cpu->core->regs[instr.rS()];
	if (instr.rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr.rA()] = result;
	return true;
}

bool PPCInstr_cntlzw(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t result = cntlzw(cpu->core->regs[instr.rS()]);
	if (instr.rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr.rA()] = result;
	return true;
}

bool PPCInstr_cmpli(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t left = cpu->core->regs[instr.rA()];
	uint32_t right = instr.uimm();
	cpu->core->cr.set(PPCCore::LT >> (4 * instr.crfD()), left < right);
	cpu->core->cr.set(PPCCore::GT >> (4 * instr.crfD()), left > right);
	cpu->core->cr.set(PPCCore::EQ >> (4 * instr.crfD()), left == right);
	return true;
}

bool PPCInstr_cmpi(PPCInterpreter *cpu, PPCInstruction instr) {
	int32_t left = cpu->core->regs[instr.rA()];
	int32_t right = instr.simm();
	cpu->core->cr.set(PPCCore::LT >> (4 * instr.crfD()), left < right);
	cpu->core->cr.set(PPCCore::GT >> (4 * instr.crfD()), left > right);
	cpu->core->cr.set(PPCCore::EQ >> (4 * instr.crfD()), left == right);
	return true;
}

bool PPCInstr_cmpl(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t left = cpu->core->regs[instr.rA()];
	uint32_t right = cpu->core->regs[instr.rB()];
	cpu->core->cr.set(PPCCore::LT >> (4 * instr.crfD()), left < right);
	cpu->core->cr.set(PPCCore::GT >> (4 * instr.crfD()), left > right);
	cpu->core->cr.set(PPCCore::EQ >> (4 * instr.crfD()), left == right);
	return true;
}

bool PPCInstr_cmp(PPCInterpreter *cpu, PPCInstruction instr) {
	int32_t left = cpu->core->regs[instr.rA()];
	int32_t right = cpu->core->regs[instr.rB()];
	cpu->core->cr.set(PPCCore::LT >> (4 * instr.crfD()), left < right);
	cpu->core->cr.set(PPCCore::GT >> (4 * instr.crfD()), left > right);
	cpu->core->cr.set(PPCCore::EQ >> (4 * instr.crfD()), left == right);
	return true;
}

bool PPCInstr_b(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t target = instr.li();
	if (!instr.aa()) {
		target += cpu->core->pc - 4;
	}
	if (instr.lk()) {
		cpu->core->lr = cpu->core->pc;
	}
	cpu->core->pc = target;
	return true;
}

bool PPCInstr_bc(PPCInterpreter *cpu, PPCInstruction instr) {
	if (PPC_CheckCondition(cpu, instr)) {
		if (instr.lk()) {
			cpu->core->lr = cpu->core->pc;
		}
		
		uint32_t target = instr.bd();
		if (!instr.aa()) {
			target += cpu->core->pc - 4;
		}
		cpu->core->pc = target;
	}
	return true;
}

bool PPCInstr_bclr(PPCInterpreter *cpu, PPCInstruction instr) {
	if (PPC_CheckCondition(cpu, instr)) {
		uint32_t target = cpu->core->lr;
		if (instr.lk()) {
			cpu->core->lr = cpu->core->pc;
		}
		cpu->core->pc = target;
	}
	return true;
}

bool PPCInstr_bcctr(PPCInterpreter *cpu, PPCInstruction instr) {
	if (PPC_CheckCondition(cpu, instr)) {
		if (instr.lk()) {
			cpu->core->lr = cpu->core->pc;
		}
		cpu->core->pc = cpu->core->ctr;
	}
	return true;
}

bool PPCInstr_lbz(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t addr = (instr.rA() ? cpu->core->regs[instr.rA()] : 0) + instr.d();
	
	uint8_t value;
	if (!cpu->read<uint8_t>(addr, &value)) return false;
	cpu->core->regs[instr.rD()] = value;
	return true;
}

bool PPCInstr_lhz(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t addr = (instr.rA() ? cpu->core->regs[instr.rA()] : 0) + instr.d();
	
	uint16_t value;
	if (!cpu->read<uint16_t>(addr, &value)) return false;
	cpu->core->regs[instr.rD()] = value;
	return true;
}

bool PPCInstr_lha(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t addr = (instr.rA() ? cpu->core->regs[instr.rA()] : 0) + instr.d();
	
	int16_t value;
	if (!cpu->read<int16_t>(addr, &value)) return false;
	cpu->core->regs[instr.rD()] = value;
	return true;
}

bool PPCInstr_lwz(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t addr = (instr.rA() ? cpu->core->regs[instr.rA()] : 0) + instr.d();
	return cpu->read<uint32_t>(addr, &cpu->core->regs[instr.rD()]);
}

bool PPCInstr_lfs(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t addr = (instr.rA() ? cpu->core->regs[instr.rA()] : 0) + instr.d();
	return cpu->read<float>(addr, &cpu->core->fprs[instr.rD()].ps0);
}

bool PPCInstr_lfd(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t addr = (instr.rA() ? cpu->core->regs[instr.rA()] : 0) + instr.d();
	return cpu->read<double>(addr, &cpu->core->fprs[instr.rD()].dbl);
}

bool PPCInstr_stb(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t addr = (instr.rA() ? cpu->core->regs[instr.rA()] : 0) + instr.d();
	return cpu->write<uint8_t>(addr, cpu->core->regs[instr.rS()]);
}

bool PPCInstr_sth(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t addr = (instr.rA() ? cpu->core->regs[instr.rA()] : 0) + instr.d();
	return cpu->write<uint16_t>(addr, cpu->core->regs[instr.rS()]);
}

bool PPCInstr_stw(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t addr = (instr.rA() ? cpu->core->regs[instr.rA()] : 0) + instr.d();
	return cpu->write<uint32_t>(addr, cpu->core->regs[instr.rS()]);
}

bool PPCInstr_stfs(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t addr = (instr.rA() ? cpu->core->regs[instr.rA()] : 0) + instr.d();
	return cpu->write<float>(addr, cpu->core->fprs[instr.rS()].ps0);
}

bool PPCInstr_stfd(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t addr = (instr.rA() ? cpu->core->regs[instr.rA()] : 0) + instr.d();
	return cpu->write<double>(addr, cpu->core->fprs[instr.rS()].dbl);
}

bool PPCInstr_lbzu(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t addr = cpu->core->regs[instr.rA()] + instr.d();
	
	uint8_t value;
	if (!cpu->read<uint8_t>(addr, &value)) return false;
	cpu->core->regs[instr.rD()] = value;
	cpu->core->regs[instr.rA()] = addr;
	return true;
}

bool PPCInstr_lhzu(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t addr = cpu->core->regs[instr.rA()] + instr.d();
	
	uint16_t value;
	if (!cpu->read<uint16_t>(addr, &value)) return false;
	cpu->core->regs[instr.rD()] = value;
	cpu->core->regs[instr.rA()] = addr;
	return true;
}

bool PPCInstr_lwzu(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t addr = cpu->core->regs[instr.rA()] + instr.d();
	cpu->core->regs[instr.rA()] = addr;
	return cpu->read<uint32_t>(addr, &cpu->core->regs[instr.rD()]);
}

bool PPCInstr_stbu(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t addr = cpu->core->regs[instr.rA()] + instr.d();
	if (!cpu->write<uint8_t>(addr, cpu->core->regs[instr.rS()])) return false;
	cpu->core->regs[instr.rA()] = addr;
	return true;
}

bool PPCInstr_sthu(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t addr = cpu->core->regs[instr.rA()] + instr.d();
	if (!cpu->write<uint16_t>(addr, cpu->core->regs[instr.rS()])) return false;
	cpu->core->regs[instr.rA()] = addr;
	return true;
}

bool PPCInstr_stwu(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t addr = cpu->core->regs[instr.rA()] + instr.d();
	if (!cpu->write<uint32_t>(addr, cpu->core->regs[instr.rS()])) return false;
	cpu->core->regs[instr.rA()] = addr;
	return true;
}

bool PPCInstr_lbzx(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t base = instr.rA() ? cpu->core->regs[instr.rA()] : 0;
	uint32_t addr = base + cpu->core->regs[instr.rB()];
	
	uint8_t value;
	if (!cpu->read<uint8_t>(addr, &value)) return false;
	cpu->core->regs[instr.rD()] = value;
	return true;
}

bool PPCInstr_lhzx(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t base = instr.rA() ? cpu->core->regs[instr.rA()] : 0;
	uint32_t addr = base + cpu->core->regs[instr.rB()];
	
	uint16_t value;
	if (!cpu->read<uint16_t>(addr, &value)) return false;
	cpu->core->regs[instr.rD()] = value;
	return true;
}

bool PPCInstr_lwzx(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t base = instr.rA() ? cpu->core->regs[instr.rA()] : 0;
	uint32_t addr = base + cpu->core->regs[instr.rB()];
	return cpu->read<uint32_t>(addr, &cpu->core->regs[instr.rD()]);
}

bool PPCInstr_stbx(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t base = instr.rA() ? cpu->core->regs[instr.rA()] : 0;
	uint32_t addr = base + cpu->core->regs[instr.rB()];
	return cpu->write<uint8_t>(addr, cpu->core->regs[instr.rS()]);
}

bool PPCInstr_sthx(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t base = instr.rA() ? cpu->core->regs[instr.rA()] : 0;
	uint32_t addr = base + cpu->core->regs[instr.rB()];
	return cpu->write<uint16_t>(addr, cpu->core->regs[instr.rS()]);
}

bool PPCInstr_stwx(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t base = instr.rA() ? cpu->core->regs[instr.rA()] : 0;
	uint32_t addr = base + cpu->core->regs[instr.rB()];
	return cpu->write<uint32_t>(addr, cpu->core->regs[instr.rS()]);
}

bool PPCInstr_lbzux(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t addr = cpu->core->regs[instr.rA()] + cpu->core->regs[instr.rB()];
	cpu->core->regs[instr.rA()] = addr;
	
	uint8_t value;
	if (!cpu->read<uint8_t>(addr, &value)) return false;
	cpu->core->regs[instr.rD()] = value;
	return true;
}

bool PPCInstr_lwzux(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t addr = cpu->core->regs[instr.rA()] + cpu->core->regs[instr.rB()];
	cpu->core->regs[instr.rA()] = addr;
	return cpu->read<uint32_t>(addr, &cpu->core->regs[instr.rD()]);
}

bool PPCInstr_stwux(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t addr = cpu->core->regs[instr.rA()] + cpu->core->regs[instr.rB()];
	if (!cpu->write<uint32_t>(addr, cpu->core->regs[instr.rS()])) return false;
	cpu->core->regs[instr.rA()] = addr;
	return true;
}

bool PPCInstr_lmw(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t addr = (instr.rA() ? cpu->core->regs[instr.rA()] : 0) + instr.d();
	for (int reg = instr.rD(); reg < 32; reg++) {
		if (!cpu->read<uint32_t>(addr, &cpu->core->regs[reg])) return false;
		addr += 4;
	}
	return true;
}

bool PPCInstr_stmw(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t addr = (instr.rA() ? cpu->core->regs[instr.rA()] : 0) + instr.d();
	for (int reg = instr.rS(); reg < 32; reg++) {
		if (!cpu->write<uint32_t>(addr, cpu->core->regs[reg])) return false;
		addr += 4;
	}
	return true;
}

bool PPCInstr_lwarx(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t base = instr.rA() ? cpu->core->regs[instr.rA()] : 0;
	uint32_t addr = base + cpu->core->regs[instr.rB()];
	cpu->core->lockMgr->reserve(cpu->core, addr);
	return cpu->read<uint32_t>(addr, &cpu->core->regs[instr.rD()]);
}

bool PPCInstr_stwcx(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t base = instr.rA() ? cpu->core->regs[instr.rA()] : 0;
	uint32_t addr = base + cpu->core->regs[instr.rB()];
	if (cpu->core->lockMgr->isReserved(cpu->core, addr)) {
		if (!cpu->write<uint32_t>(addr, cpu->core->regs[instr.rS()])) return false;
		cpu->core->cr.set(PPCCore::EQ, true);
		cpu->core->lockMgr->reset();
	}
	else {
		cpu->core->cr.set(PPCCore::EQ, false);
	}
	cpu->core->cr.set(PPCCore::LT | PPCCore::GT, false);
	return true;
}

bool PPCInstr_crxor(PPCInterpreter *cpu, PPCInstruction instr) {
	cpu->core->cr.set(1 << instr.crbD(),
		cpu->core->cr.get(1 << instr.crbA()) ^ cpu->core->cr.get(1 << instr.crbB())
	);
	return true;
}

bool PPCInstr_mfcr(PPCInterpreter *cpu, PPCInstruction instr) {
	cpu->core->regs[instr.rD()] = cpu->core->cr;
	return true;
}

bool PPCInstr_mtcrf(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t mask = 0;
	for (int i = 7; i >= 0; i--) {
		mask <<= 4;
		if (instr.crm() & (1 << i)) {
			mask |= 0xF;
		}
	}
	
	uint32_t value = cpu->core->regs[instr.rS()] & mask;
	cpu->core->cr.value = (cpu->core->cr.value & ~mask) | value;
	return true;
}

bool PPCInstr_mftb(PPCInterpreter *cpu, PPCInstruction instr) {
	return cpu->core->getSpr((PPCCore::SPR)instr.spr(), &cpu->core->regs[instr.rD()]);
}

bool PPCInstr_mfspr(PPCInterpreter *cpu, PPCInstruction instr) {
	return cpu->core->getSpr((PPCCore::SPR)instr.spr(), &cpu->core->regs[instr.rD()]);
}

bool PPCInstr_mtspr(PPCInterpreter *cpu, PPCInstruction instr) {
	return cpu->core->setSpr((PPCCore::SPR)instr.spr(), cpu->core->regs[instr.rS()]);
}

bool PPCInstr_sc(PPCInterpreter *cpu, PPCInstruction instr) {
	return cpu->core->triggerException(PPCCore::SystemCall);
}

/********** SUPERVISOR INSTRUCTIONS **********/

bool PPCInstr_mfmsr(PPCInterpreter *cpu, PPCInstruction instr) {
	cpu->core->regs[instr.rD()] = cpu->core->msr;
	return true;
}

bool PPCInstr_mtmsr(PPCInterpreter *cpu, PPCInstruction instr) {
	return cpu->core->setMsr(cpu->core->regs[instr.rS()]);
}

bool PPCInstr_mfsr(PPCInterpreter *cpu, PPCInstruction instr) {
	return cpu->core->getSr(instr.sr(), &cpu->core->regs[instr.rD()]);
}

bool PPCInstr_mtsr(PPCInterpreter *cpu, PPCInstruction instr) {
	return cpu->core->setSr(instr.sr(), cpu->core->regs[instr.rS()]);
}

bool PPCInstr_tlbie(PPCInterpreter *cpu, PPCInstruction instr) {
	cpu->invalidateMMUCache();
	return true;
}

bool PPCInstr_rfi(PPCInterpreter *cpu, PPCInstruction instr) {
	cpu->core->pc = cpu->core->srr0;
	if (!cpu->core->setMsr(cpu->core->srr1)) return false;
	return true;
}

/********** CACHE AND SYNCHRONIZATION INSTRUCTIONS **********/

bool PPCInstr_sync(PPCInterpreter *cpu, PPCInstruction instr) { return true; }
bool PPCInstr_isync(PPCInterpreter *cpu, PPCInstruction instr) { return true; }
bool PPCInstr_eieio(PPCInterpreter *cpu, PPCInstruction instr) { return true; }
bool PPCInstr_dcbf(PPCInterpreter *cpu, PPCInstruction instr) { return true; }
bool PPCInstr_dcbi(PPCInterpreter *cpu, PPCInstruction instr) { return true; }
bool PPCInstr_dcbz(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t base = instr.rA() ? cpu->core->regs[instr.rA()] : 0;
	uint32_t addr = (base + cpu->core->regs[instr.rB()]) & ~0x1F;
	for (int i = 0; i < 4; i++) {
		if (!cpu->write<uint64_t>(addr, 0)) return false;
		addr += 8;
	}
	return true;
}
bool PPCInstr_dcbz_l(PPCInterpreter *cpu, PPCInstruction instr) { return true; }
bool PPCInstr_dcbst(PPCInterpreter *cpu, PPCInstruction instr) { return true; }
bool PPCInstr_icbi(PPCInterpreter *cpu, PPCInstruction instr) {
	cpu->invalidateICache();
	return true;
}

/********** FLOATING POINT INSTRUCTIONS **********/

bool PPCInstr_frsp(PPCInterpreter *cpu, PPCInstruction instr) {
	cpu->core->fprs[instr.rD()].ps0 = (float)cpu->core->fprs[instr.rB()].dbl;
	return true;
}

bool PPCInstr_fmuls(PPCInterpreter *cpu, PPCInstruction instr) {
	cpu->core->fprs[instr.rD()].ps0 = cpu->core->fprs[instr.rA()].ps0 * cpu->core->fprs[instr.rC()].ps0;
	return true;
}

/********** PAIRED SINGLE INSTRUCTIONS **********/

template <class T>
bool PSQ_LoadTmpl(PPCInterpreter *cpu, uint32_t addr, int rD, int scale, int w) {
	NotImplementedError("PSQ_LoadTmpl");
	return false;
}

bool PSQ_Load(PPCInterpreter *cpu, uint32_t addr, int rD, int i, int w) {
	uint32_t config = cpu->core->gqrs[i];
	int scale = (config >> 24) & 0x3F;
	int type = (config >> 16) & 7;
	if (type & 4) { //Dequantize
		if (type & 2) { //Signed
			if (type & 1) return PSQ_LoadTmpl<int16_t>(cpu, addr, rD, scale, w);
			return PSQ_LoadTmpl<int8_t>(cpu, addr, rD, scale, w);
		}
		if (type & 1) return PSQ_LoadTmpl<uint16_t>(cpu, addr, rD, scale, w);
		return PSQ_LoadTmpl<uint8_t>(cpu, addr, rD, scale, w);
	}
	
	if (w) {
		if (!cpu->read<float>(addr, &cpu->core->fprs[i].ps0)) return false;
		cpu->core->fprs[i].ps1 = 1.0;
	}
	else {
		if (!cpu->read<float>(addr, &cpu->core->fprs[i].ps0)) return false;
		if (!cpu->read<float>(addr+4, &cpu->core->fprs[i].ps1)) return false;
	}
	return true;
}

bool PPCInstr_psq_l(PPCInterpreter *cpu, PPCInstruction instr) {
	uint32_t addr = (instr.rA() ? cpu->core->regs[instr.rA()] : 0) + instr.ps_d();
	return PSQ_Load(cpu, addr, instr.rD(), instr.ps_i(), instr.ps_w());
}

PPCInstrCallback PPCInstruction::decode() {
	switch(opcode()) {
		case 4: return PPCInstr_dcbz_l;
		case 7: return PPCInstr_mulli;
		case 8: return PPCInstr_subfic;
		case 10: return PPCInstr_cmpli;
		case 11: return PPCInstr_cmpi;
		case 12: return PPCInstr_addic;
		case 13: return PPCInstr_addic_rc;
		case 14: return PPCInstr_addi;
		case 15: return PPCInstr_addis;
		case 16: return PPCInstr_bc;
		case 17: return PPCInstr_sc;
		case 18: return PPCInstr_b;
		case 19:
			switch(opcode2()) {
				case 16: return PPCInstr_bclr;
				case 50: return PPCInstr_rfi;
				case 150: return PPCInstr_isync;
				case 193: return PPCInstr_crxor;
				case 528: return PPCInstr_bcctr;
				default:
					NotImplementedError("PPC opcode 19: %i", opcode2());
					return NULL;
			}
		case 20: return PPCInstr_rlwimi;
		case 21: return PPCInstr_rlwinm;
		case 24: return PPCInstr_ori;
		case 25: return PPCInstr_oris;
		case 26: return PPCInstr_xori;
		case 27: return PPCInstr_xoris;
		case 28: return PPCInstr_andi;
		case 29: return PPCInstr_andis;
		case 31:
			switch(opcode2()) {
				case 0: return PPCInstr_cmp;
				case 8: return PPCInstr_subfc;
				case 10: return PPCInstr_addc;
				case 11: return PPCInstr_mulhwu;
				case 19: return PPCInstr_mfcr;
				case 20: return PPCInstr_lwarx;
				case 23: return PPCInstr_lwzx;
				case 24: return PPCInstr_slw;
				case 26: return PPCInstr_cntlzw;
				case 28: return PPCInstr_and;
				case 32: return PPCInstr_cmpl;
				case 40: return PPCInstr_subf;
				case 54: return PPCInstr_dcbst;
				case 55: return PPCInstr_lwzux;
				case 60: return PPCInstr_andc;
				case 75: return PPCInstr_mulhw;
				case 83: return PPCInstr_mfmsr;
				case 86: return PPCInstr_dcbf;
				case 87: return PPCInstr_lbzx;
				case 104: return PPCInstr_neg;
				case 119: return PPCInstr_lbzux;
				case 124: return PPCInstr_nor;
				case 136: return PPCInstr_subfe;
				case 138: return PPCInstr_adde;
				case 144: return PPCInstr_mtcrf;
				case 146: return PPCInstr_mtmsr;
				case 150: return PPCInstr_stwcx;
				case 151: return PPCInstr_stwx;
				case 183: return PPCInstr_stwux;
				case 200: return PPCInstr_subfze;
				case 202: return PPCInstr_addze;
				case 210: return PPCInstr_mtsr;
				case 215: return PPCInstr_stbx;
				case 235: return PPCInstr_mullw;
				case 266: return PPCInstr_add;
				case 279: return PPCInstr_lhzx;
				case 306: return PPCInstr_tlbie;
				case 316: return PPCInstr_xor;
				case 339: return PPCInstr_mfspr;
				case 371: return PPCInstr_mftb;
				case 407: return PPCInstr_sthx;
				case 444: return PPCInstr_or;
				case 459: return PPCInstr_divwu;
				case 467: return PPCInstr_mtspr;
				case 470: return PPCInstr_dcbi;
				case 491: return PPCInstr_divw;
				case 536: return PPCInstr_srw;
				case 595: return PPCInstr_mfsr;
				case 598: return PPCInstr_sync;
				case 792: return PPCInstr_sraw;
				case 824: return PPCInstr_srawi;
				case 854: return PPCInstr_eieio;
				case 922: return PPCInstr_extsh;
				case 954: return PPCInstr_extsb;
				case 982: return PPCInstr_icbi;
				case 1014: return PPCInstr_dcbz;
				default:
					NotImplementedError("PPC opcode 31: %i", opcode2());
					return NULL;
			}
		case 32: return PPCInstr_lwz;
		case 33: return PPCInstr_lwzu;
		case 34: return PPCInstr_lbz;
		case 35: return PPCInstr_lbzu;
		case 36: return PPCInstr_stw;
		case 37: return PPCInstr_stwu;
		case 38: return PPCInstr_stb;
		case 39: return PPCInstr_stbu;
		case 40: return PPCInstr_lhz;
		case 41: return PPCInstr_lhzu;
		case 42: return PPCInstr_lha;
		case 44: return PPCInstr_sth;
		case 45: return PPCInstr_sthu;
		case 46: return PPCInstr_lmw;
		case 47: return PPCInstr_stmw;
		case 48: return PPCInstr_lfs;
		case 50: return PPCInstr_lfd;
		case 52: return PPCInstr_stfs;
		case 54: return PPCInstr_stfd;
		case 56: return PPCInstr_psq_l;
		case 59:
			switch(opcode3()) {
				case 25: return PPCInstr_fmuls;
				default:
					NotImplementedError("PPC opcode 59: %i", opcode3());
					return NULL;
			}
		case 63:
			switch(opcode2()) {
				case 12: return PPCInstr_frsp;
				default:
					NotImplementedError("PPC opcode 63: %i", opcode2());
					return NULL;
			}
		default:
			NotImplementedError("PPC opcode %i", opcode());
			return NULL;
	}
}
