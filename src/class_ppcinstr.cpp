
#include "class_ppcinstr.h"
#include "class_ppcinterp.h"
#include "class_ppccore.h"
#include "class_endian.h"
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

bool PPC_CheckCondition(PPCInstruction *instr, PPCInterpreter *cpu) {
	int bo = instr->bo();
	if (!(bo & 4)) {
		cpu->core->ctr--;
		if ((bo & 2) && cpu->core->ctr != 0) return false;
		else if (cpu->core->ctr == 0) return false;
	}
	if (bo & 0x10) return true;
	if (bo & 8) return (cpu->core->cr >> (31 - instr->bi())) & 1;
	return !((cpu->core->cr >> (31 - instr->bi())) & 1);
}

void PPC_UpdateConditions(PPCCore *core, uint32_t result) {
	core->cr.set(PPCCore::LT, result >> 31);
	core->cr.set(PPCCore::GT, !(result >> 31) && result != 0);
	core->cr.set(PPCCore::EQ, result == 0);
}

/********** NORMAL INSTRUCTIONS **********/

bool PPCInstr_addi(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t source = instr->rA() ? cpu->core->regs[instr->rA()] : 0;
	cpu->core->regs[instr->rD()] = source + instr->simm();
	return true;
}

bool PPCInstr_addis(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t source = instr->rA() ? cpu->core->regs[instr->rA()] : 0;
	cpu->core->regs[instr->rD()] = source + (instr->simm() << 16);
	return true;
}

bool PPCInstr_mulli(PPCInstruction *instr, PPCInterpreter *cpu) {
	cpu->core->regs[instr->rD()] = cpu->core->regs[instr->rA()] * instr->simm();
	return true;
}

bool PPCInstr_ori(PPCInstruction *instr, PPCInterpreter *cpu) {
	cpu->core->regs[instr->rA()] = cpu->core->regs[instr->rS()] | instr->uimm();
	return true;
}

bool PPCInstr_oris(PPCInstruction *instr, PPCInterpreter *cpu) {
	cpu->core->regs[instr->rA()] = cpu->core->regs[instr->rS()] | (instr->uimm() << 16);
	return true;
}

bool PPCInstr_andi(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t result = cpu->core->regs[instr->rS()] & instr->uimm();
	PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr->rA()] = result;
	return true;
}

bool PPCInstr_andis(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t result = cpu->core->regs[instr->rS()] & (instr->uimm() << 16);
	PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr->rA()] = result;
	return true;
}

bool PPCInstr_xori(PPCInstruction *instr, PPCInterpreter *cpu) {
	cpu->core->regs[instr->rA()] = cpu->core->regs[instr->rS()] ^ instr->uimm();
	return true;
}

bool PPCInstr_xoris(PPCInstruction *instr, PPCInterpreter *cpu) {
	cpu->core->regs[instr->rA()] = cpu->core->regs[instr->rS()] ^ (instr->uimm() << 16);
	return true;
}

bool PPCInstr_addic(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t left = cpu->core->regs[instr->rA()];
	uint32_t right = instr->simm();
	uint64_t result = (uint64_t)left + right;
	cpu->core->xer.set(PPCCore::CA, result >> 32);
	cpu->core->regs[instr->rD()] = (uint32_t)result;
	return true;
}

bool PPCInstr_addic_rc(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t left = cpu->core->regs[instr->rA()];
	uint32_t right = instr->simm();
	uint64_t result = (uint64_t)left + right;
	cpu->core->xer.set(PPCCore::CA, result >> 32);
	PPC_UpdateConditions(cpu->core, (uint32_t)result);
	cpu->core->regs[instr->rD()] = (uint32_t)result;
	return true;
}

bool PPCInstr_subfic(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t left = cpu->core->regs[instr->rA()];
	uint32_t right = instr->simm();
	uint64_t result = (uint64_t)~left + right + 1;
	cpu->core->xer.set(PPCCore::CA, result >> 32);
	cpu->core->regs[instr->rD()] = (uint32_t)result;
	return true;
}

bool PPCInstr_srawi(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t value = cpu->core->regs[instr->rS()];
	uint32_t result = (int32_t)value >> instr->sh();

	if (instr->sh() && (value >> 31)) {
		cpu->core->xer.set(PPCCore::CA, value & genmask(32 - instr->sh(), 31));
	}
	else {
		cpu->core->xer.set(PPCCore::CA, 0);
	}

	if (instr->rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr->rA()] = result;
	return true;
}

bool PPCInstr_add(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t result = cpu->core->regs[instr->rA()] + cpu->core->regs[instr->rB()];
	if (instr->rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr->rD()] = result;
	return true;
}

bool PPCInstr_addc(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t left = cpu->core->regs[instr->rA()];
	uint32_t right = cpu->core->regs[instr->rB()];
	uint64_t result = (uint64_t)left + right;
	cpu->core->xer.set(PPCCore::CA, result >> 32);
	if (instr->rc()) PPC_UpdateConditions(cpu->core, (uint32_t)result);
	cpu->core->regs[instr->rD()] = (uint32_t)result;
	return true;
}

bool PPCInstr_adde(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint64_t result = (uint64_t)cpu->core->regs[instr->rA()] + cpu->core->regs[instr->rB()] + cpu->core->xer.get(PPCCore::CA);
	cpu->core->xer.set(PPCCore::CA, result >> 32);
	if (instr->rc()) PPC_UpdateConditions(cpu->core, (uint32_t)result);
	cpu->core->regs[instr->rD()] = (uint32_t)result;
	return true;
}

bool PPCInstr_addze(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint64_t result = (uint64_t)cpu->core->regs[instr->rA()] + cpu->core->xer.get(PPCCore::CA);
	cpu->core->xer.set(PPCCore::CA, result >> 32);
	if (instr->rc()) PPC_UpdateConditions(cpu->core, (uint32_t)result);
	cpu->core->regs[instr->rD()] = (uint32_t)result;
	return true;
}

bool PPCInstr_subf(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t result = cpu->core->regs[instr->rB()] - cpu->core->regs[instr->rA()];
	if (instr->rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr->rD()] = result;
	return true;
}

bool PPCInstr_subfc(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t left = cpu->core->regs[instr->rA()];
	uint32_t right = cpu->core->regs[instr->rB()];
	uint64_t result = (uint64_t)~left + right + 1;
	cpu->core->xer.set(PPCCore::CA, result >> 32);
	if (instr->rc()) PPC_UpdateConditions(cpu->core, (uint32_t)result);
	cpu->core->regs[instr->rD()] = (uint32_t)result;
	return true;
}

bool PPCInstr_subfe(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint64_t result = (uint64_t)~cpu->core->regs[instr->rA()] + cpu->core->regs[instr->rB()] + cpu->core->xer.get(PPCCore::CA);
	cpu->core->xer.set(PPCCore::CA, result >> 32);
	if (instr->rc()) PPC_UpdateConditions(cpu->core, (uint32_t)result);
	cpu->core->regs[instr->rD()] = (uint32_t)result;
	return true;
}

bool PPCInstr_subfze(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint64_t result = (uint64_t)~cpu->core->regs[instr->rA()] + cpu->core->xer.get(PPCCore::CA);
	cpu->core->xer.set(PPCCore::CA, result >> 32);
	if (instr->rc()) PPC_UpdateConditions(cpu->core, (uint32_t)result);
	cpu->core->regs[instr->rD()] = (uint32_t)result;
	return true;
}

bool PPCInstr_mullw(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t result = cpu->core->regs[instr->rA()] * cpu->core->regs[instr->rB()];
	if (instr->rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr->rD()] = result;
	return true;
}

bool PPCInstr_mulhw(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint64_t result = (int64_t)cpu->core->regs[instr->rA()] * (int64_t)cpu->core->regs[instr->rB()];
	if (instr->rc()) PPC_UpdateConditions(cpu->core, result >> 32);
	cpu->core->regs[instr->rD()] = result >> 32;
	return true;
}

bool PPCInstr_mulhwu(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint64_t result = (uint64_t)cpu->core->regs[instr->rA()] * cpu->core->regs[instr->rB()];
	if (instr->rc()) PPC_UpdateConditions(cpu->core, result >> 32);
	cpu->core->regs[instr->rD()] = result >> 32;
	return true;
}

bool PPCInstr_divwu(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t dividend = cpu->core->regs[instr->rA()];
	uint32_t divisor = cpu->core->regs[instr->rB()];
	if (divisor != 0) {
		uint32_t result = dividend / divisor;
		if (instr->rc()) PPC_UpdateConditions(cpu->core, result);
		cpu->core->regs[instr->rD()] = result;
	}
	return true;
}

bool PPCInstr_divw(PPCInstruction *instr, PPCInterpreter *cpu) {
	int32_t dividend = cpu->core->regs[instr->rA()];
	int32_t divisor = cpu->core->regs[instr->rB()];
	if (divisor != 0) {
		int32_t result = dividend / divisor;
		if (instr->rc()) PPC_UpdateConditions(cpu->core, result);
		cpu->core->regs[instr->rD()] = result;
	}
	return true;
}

bool PPCInstr_or(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t result = cpu->core->regs[instr->rS()] | cpu->core->regs[instr->rB()];
	if (instr->rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr->rA()] = result;
	return true;
}

bool PPCInstr_and(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t result = cpu->core->regs[instr->rS()] & cpu->core->regs[instr->rB()];
	if (instr->rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr->rA()] = result;
	return true;
}

bool PPCInstr_xor(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t result = cpu->core->regs[instr->rS()] ^ cpu->core->regs[instr->rB()];
	if (instr->rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr->rA()] = result;
	return true;
}

bool PPCInstr_nor(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t result = ~(cpu->core->regs[instr->rS()] | cpu->core->regs[instr->rB()]);
	if (instr->rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr->rA()] = result;
	return true;
}

bool PPCInstr_andc(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t result = cpu->core->regs[instr->rS()] & ~cpu->core->regs[instr->rB()];
	if (instr->rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr->rA()] = result;
	return true;
}

bool PPCInstr_slw(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t bits = cpu->core->regs[instr->rB()] & 0x3F;
	uint32_t result = 0;
	if (!(bits & 0x20)) {
		result = cpu->core->regs[instr->rS()] << bits;
	}
	if (instr->rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr->rA()] = result;
	return true;
}

bool PPCInstr_srw(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t bits = cpu->core->regs[instr->rB()] & 0x3F;
	uint32_t result = 0;
	if (!(bits & 0x20)) {
		result = cpu->core->regs[instr->rS()] >> bits;
	}
	if (instr->rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr->rA()] = result;
	return true;
}

bool PPCInstr_sraw(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t value = cpu->core->regs[instr->rS()];
	uint32_t bits = cpu->core->regs[instr->rB()] & 0x3F;
	
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
	
	if (instr->rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr->rA()] = result;
	return true;
}

bool PPCInstr_rlwinm(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t result = rotl(cpu->core->regs[instr->rS()], instr->sh()) & genmask(instr->mb(), instr->me());
	if (instr->rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr->rA()] = result;
	return true;
}

bool PPCInstr_rlwimi(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t mask = genmask(instr->mb(), instr->me());
	uint32_t temp = rotl(cpu->core->regs[instr->rS()], instr->sh()) & mask;
	uint32_t result = (cpu->core->regs[instr->rA()] & ~mask) | temp;
	if (instr->rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr->rA()] = result;
	return true;
}

bool PPCInstr_neg(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t result = -(int32_t)cpu->core->regs[instr->rA()];
	if (instr->rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr->rD()] = result;
	return true;
}

bool PPCInstr_extsb(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t result = (int8_t)cpu->core->regs[instr->rS()];
	if (instr->rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr->rA()] = result;
	return true;
}

bool PPCInstr_extsh(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t result = (int16_t)cpu->core->regs[instr->rS()];
	if (instr->rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr->rA()] = result;
	return true;
}

bool PPCInstr_cntlzw(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t result = cntlzw(cpu->core->regs[instr->rS()]);
	if (instr->rc()) PPC_UpdateConditions(cpu->core, result);
	cpu->core->regs[instr->rA()] = result;
	return true;
}

bool PPCInstr_cmpli(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t left = cpu->core->regs[instr->rA()];
	uint32_t right = instr->uimm();
	cpu->core->cr.set(PPCCore::LT >> (4 * instr->crfD()), left < right);
	cpu->core->cr.set(PPCCore::GT >> (4 * instr->crfD()), left > right);
	cpu->core->cr.set(PPCCore::EQ >> (4 * instr->crfD()), left == right);
	return true;
}

bool PPCInstr_cmpi(PPCInstruction *instr, PPCInterpreter *cpu) {
	int32_t left = cpu->core->regs[instr->rA()];
	int32_t right = instr->simm();
	cpu->core->cr.set(PPCCore::LT >> (4 * instr->crfD()), left < right);
	cpu->core->cr.set(PPCCore::GT >> (4 * instr->crfD()), left > right);
	cpu->core->cr.set(PPCCore::EQ >> (4 * instr->crfD()), left == right);
	return true;
}

bool PPCInstr_cmpl(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t left = cpu->core->regs[instr->rA()];
	uint32_t right = cpu->core->regs[instr->rB()];
	cpu->core->cr.set(PPCCore::LT >> (4 * instr->crfD()), left < right);
	cpu->core->cr.set(PPCCore::GT >> (4 * instr->crfD()), left > right);
	cpu->core->cr.set(PPCCore::EQ >> (4 * instr->crfD()), left == right);
	return true;
}

bool PPCInstr_cmp(PPCInstruction *instr, PPCInterpreter *cpu) {
	int32_t left = cpu->core->regs[instr->rA()];
	int32_t right = cpu->core->regs[instr->rB()];
	cpu->core->cr.set(PPCCore::LT >> (4 * instr->crfD()), left < right);
	cpu->core->cr.set(PPCCore::GT >> (4 * instr->crfD()), left > right);
	cpu->core->cr.set(PPCCore::EQ >> (4 * instr->crfD()), left == right);
	return true;
}

bool PPCInstr_b(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t target = instr->li();
	if (!instr->aa()) {
		target += cpu->core->pc - 4;
	}
	if (instr->lk()) {
		cpu->core->lr = cpu->core->pc;
	}
	cpu->core->pc = target;
	return true;
}

bool PPCInstr_bc(PPCInstruction *instr, PPCInterpreter *cpu) {
	if (PPC_CheckCondition(instr, cpu)) {
		if (instr->lk()) {
			cpu->core->lr = cpu->core->pc;
		}
		
		uint32_t target = instr->bd();
		if (!instr->aa()) {
			target += cpu->core->pc - 4;
		}
		cpu->core->pc = target;
	}
	return true;
}

bool PPCInstr_bclr(PPCInstruction *instr, PPCInterpreter *cpu) {
	if (PPC_CheckCondition(instr, cpu)) {
		uint32_t target = cpu->core->lr;
		if (instr->lk()) {
			cpu->core->lr = cpu->core->pc;
		}
		cpu->core->pc = target;
	}
	return true;
}

bool PPCInstr_bcctr(PPCInstruction *instr, PPCInterpreter *cpu) {
	if (PPC_CheckCondition(instr, cpu)) {
		if (instr->lk()) {
			cpu->core->lr = cpu->core->pc;
		}
		cpu->core->pc = cpu->core->ctr;
	}
	return true;
}

bool PPCInstr_lbz(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t addr = (instr->rA() ? cpu->core->regs[instr->rA()] : 0) + instr->d();
	
	uint8_t value;
	if (!cpu->read<uint8_t>(addr, &value)) return false;
	cpu->core->regs[instr->rD()] = value;
	return true;
}

bool PPCInstr_lhz(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t addr = (instr->rA() ? cpu->core->regs[instr->rA()] : 0) + instr->d();
	
	uint16_t value;
	if (!cpu->read<uint16_t>(addr, &value)) return false;
	cpu->core->regs[instr->rD()] = value;
	return true;
}

bool PPCInstr_lha(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t addr = (instr->rA() ? cpu->core->regs[instr->rA()] : 0) + instr->d();
	
	int16_t value;
	if (!cpu->read<int16_t>(addr, &value)) return false;
	cpu->core->regs[instr->rD()] = value;
	return true;
}

bool PPCInstr_lwz(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t addr = (instr->rA() ? cpu->core->regs[instr->rA()] : 0) + instr->d();
	return cpu->read<uint32_t>(addr, &cpu->core->regs[instr->rD()]);
}

bool PPCInstr_lfs(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t addr = (instr->rA() ? cpu->core->regs[instr->rA()] : 0) + instr->d();
	return cpu->read<float>(addr, &cpu->core->fprs[instr->rD()].ps0);
}

bool PPCInstr_lfd(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t addr = (instr->rA() ? cpu->core->regs[instr->rA()] : 0) + instr->d();
	return cpu->read<double>(addr, &cpu->core->fprs[instr->rD()].dbl);
}

bool PPCInstr_stb(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t addr = (instr->rA() ? cpu->core->regs[instr->rA()] : 0) + instr->d();
	return cpu->write<uint8_t>(addr, cpu->core->regs[instr->rS()]);
}

bool PPCInstr_sth(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t addr = (instr->rA() ? cpu->core->regs[instr->rA()] : 0) + instr->d();
	return cpu->write<uint16_t>(addr, cpu->core->regs[instr->rS()]);
}

bool PPCInstr_stw(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t addr = (instr->rA() ? cpu->core->regs[instr->rA()] : 0) + instr->d();
	return cpu->write<uint32_t>(addr, cpu->core->regs[instr->rS()]);
}

bool PPCInstr_stfs(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t addr = (instr->rA() ? cpu->core->regs[instr->rA()] : 0) + instr->d();
	return cpu->write<float>(addr, cpu->core->fprs[instr->rS()].ps0);
}

bool PPCInstr_stfd(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t addr = (instr->rA() ? cpu->core->regs[instr->rA()] : 0) + instr->d();
	return cpu->write<double>(addr, cpu->core->fprs[instr->rS()].dbl);
}

bool PPCInstr_lbzu(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t addr = cpu->core->regs[instr->rA()] + instr->d();
	
	uint8_t value;
	if (!cpu->read<uint8_t>(addr, &value)) return false;
	cpu->core->regs[instr->rD()] = value;
	cpu->core->regs[instr->rA()] = addr;
	return true;
}

bool PPCInstr_lhzu(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t addr = cpu->core->regs[instr->rA()] + instr->d();
	
	uint16_t value;
	if (!cpu->read<uint16_t>(addr, &value)) return false;
	cpu->core->regs[instr->rD()] = value;
	cpu->core->regs[instr->rA()] = addr;
	return true;
}

bool PPCInstr_lwzu(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t addr = cpu->core->regs[instr->rA()] + instr->d();
	cpu->core->regs[instr->rA()] = addr;
	return cpu->read<uint32_t>(addr, &cpu->core->regs[instr->rD()]);
}

bool PPCInstr_lfsu(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t addr = cpu->core->regs[instr->rA()] + instr->d();
	cpu->core->regs[instr->rA()] = addr;
	return cpu->read<float>(addr, &cpu->core->fprs[instr->rD()].ps0);
}

bool PPCInstr_stbu(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t addr = cpu->core->regs[instr->rA()] + instr->d();
	if (!cpu->write<uint8_t>(addr, cpu->core->regs[instr->rS()])) return false;
	cpu->core->regs[instr->rA()] = addr;
	return true;
}

bool PPCInstr_sthu(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t addr = cpu->core->regs[instr->rA()] + instr->d();
	if (!cpu->write<uint16_t>(addr, cpu->core->regs[instr->rS()])) return false;
	cpu->core->regs[instr->rA()] = addr;
	return true;
}

bool PPCInstr_stwu(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t addr = cpu->core->regs[instr->rA()] + instr->d();
	if (!cpu->write<uint32_t>(addr, cpu->core->regs[instr->rS()])) return false;
	cpu->core->regs[instr->rA()] = addr;
	return true;
}

bool PPCInstr_stfsu(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t addr = cpu->core->regs[instr->rA()] + instr->d();
	if (!cpu->write<float>(addr, cpu->core->fprs[instr->rS()].ps0)) return false;
	cpu->core->regs[instr->rA()] = addr;
	return true;
}

bool PPCInstr_lbzx(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t base = instr->rA() ? cpu->core->regs[instr->rA()] : 0;
	uint32_t addr = base + cpu->core->regs[instr->rB()];
	
	uint8_t value;
	if (!cpu->read<uint8_t>(addr, &value)) return false;
	cpu->core->regs[instr->rD()] = value;
	return true;
}

bool PPCInstr_lhzx(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t base = instr->rA() ? cpu->core->regs[instr->rA()] : 0;
	uint32_t addr = base + cpu->core->regs[instr->rB()];
	
	uint16_t value;
	if (!cpu->read<uint16_t>(addr, &value)) return false;
	cpu->core->regs[instr->rD()] = value;
	return true;
}

bool PPCInstr_lwzx(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t base = instr->rA() ? cpu->core->regs[instr->rA()] : 0;
	uint32_t addr = base + cpu->core->regs[instr->rB()];
	return cpu->read<uint32_t>(addr, &cpu->core->regs[instr->rD()]);
}

bool PPCInstr_lwbrx(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t base = instr->rA() ? cpu->core->regs[instr->rA()] : 0;
	uint32_t addr = base + cpu->core->regs[instr->rB()];
	
	uint32_t value;
	if (!cpu->read<uint32_t>(addr, &value)) return false;
	cpu->core->regs[instr->rD()] = Endian::swap32(value);
	return true;
}

bool PPCInstr_lfsx(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t base = instr->rA() ? cpu->core->regs[instr->rA()] : 0;
	uint32_t addr = base + cpu->core->regs[instr->rB()];
	return cpu->read<float>(addr, &cpu->core->fprs[instr->rD()].ps0);
}

bool PPCInstr_lfdx(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t base = instr->rA() ? cpu->core->regs[instr->rA()] : 0;
	uint32_t addr = base + cpu->core->regs[instr->rB()];
	return cpu->read<double>(addr, &cpu->core->fprs[instr->rD()].dbl);
}

bool PPCInstr_stbx(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t base = instr->rA() ? cpu->core->regs[instr->rA()] : 0;
	uint32_t addr = base + cpu->core->regs[instr->rB()];
	return cpu->write<uint8_t>(addr, cpu->core->regs[instr->rS()]);
}

bool PPCInstr_sthx(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t base = instr->rA() ? cpu->core->regs[instr->rA()] : 0;
	uint32_t addr = base + cpu->core->regs[instr->rB()];
	return cpu->write<uint16_t>(addr, cpu->core->regs[instr->rS()]);
}

bool PPCInstr_stwx(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t base = instr->rA() ? cpu->core->regs[instr->rA()] : 0;
	uint32_t addr = base + cpu->core->regs[instr->rB()];
	return cpu->write<uint32_t>(addr, cpu->core->regs[instr->rS()]);
}

bool PPCInstr_stwbrx(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t base = instr->rA() ? cpu->core->regs[instr->rA()] : 0;
	uint32_t addr = base + cpu->core->regs[instr->rB()];
	uint32_t value = Endian::swap32(cpu->core->regs[instr->rS()]);
	return cpu->write<uint32_t>(addr, value);
}

bool PPCInstr_stfsx(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t base = instr->rA() ? cpu->core->regs[instr->rA()] : 0;
	uint32_t addr = base + cpu->core->regs[instr->rB()];
	return cpu->write<float>(addr, cpu->core->fprs[instr->rS()].ps0);
}

bool PPCInstr_stfdx(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t base = instr->rA() ? cpu->core->regs[instr->rA()] : 0;
	uint32_t addr = base + cpu->core->regs[instr->rB()];
	return cpu->write<double>(addr, cpu->core->fprs[instr->rS()].dbl);
}

bool PPCInstr_stfiwx(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t base = instr->rA() ? cpu->core->regs[instr->rA()] : 0;
	uint32_t addr = base + cpu->core->regs[instr->rB()];
	return cpu->write<int32_t>(addr, cpu->core->fprs[instr->rS()].iw1);
}

bool PPCInstr_lbzux(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t addr = cpu->core->regs[instr->rA()] + cpu->core->regs[instr->rB()];
	cpu->core->regs[instr->rA()] = addr;
	
	uint8_t value;
	if (!cpu->read<uint8_t>(addr, &value)) return false;
	cpu->core->regs[instr->rD()] = value;
	return true;
}

bool PPCInstr_lwzux(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t addr = cpu->core->regs[instr->rA()] + cpu->core->regs[instr->rB()];
	cpu->core->regs[instr->rA()] = addr;
	return cpu->read<uint32_t>(addr, &cpu->core->regs[instr->rD()]);
}

bool PPCInstr_stbux(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t addr = cpu->core->regs[instr->rA()] + cpu->core->regs[instr->rB()];
	if (!cpu->write<uint8_t>(addr, cpu->core->regs[instr->rS()])) return false;
	cpu->core->regs[instr->rA()] = addr;
	return true;
}

bool PPCInstr_sthux(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t addr = cpu->core->regs[instr->rA()] + cpu->core->regs[instr->rB()];
	if (!cpu->write<uint16_t>(addr, cpu->core->regs[instr->rS()])) return false;
	cpu->core->regs[instr->rA()] = addr;
	return true;
}

bool PPCInstr_stwux(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t addr = cpu->core->regs[instr->rA()] + cpu->core->regs[instr->rB()];
	if (!cpu->write<uint32_t>(addr, cpu->core->regs[instr->rS()])) return false;
	cpu->core->regs[instr->rA()] = addr;
	return true;
}

bool PPCInstr_lmw(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t addr = (instr->rA() ? cpu->core->regs[instr->rA()] : 0) + instr->d();
	for (int reg = instr->rD(); reg < 32; reg++) {
		if (!cpu->read<uint32_t>(addr, &cpu->core->regs[reg])) return false;
		addr += 4;
	}
	return true;
}

bool PPCInstr_stmw(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t addr = (instr->rA() ? cpu->core->regs[instr->rA()] : 0) + instr->d();
	for (int reg = instr->rS(); reg < 32; reg++) {
		if (!cpu->write<uint32_t>(addr, cpu->core->regs[reg])) return false;
		addr += 4;
	}
	return true;
}

bool PPCInstr_lwarx(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t base = instr->rA() ? cpu->core->regs[instr->rA()] : 0;
	uint32_t addr = base + cpu->core->regs[instr->rB()];
	cpu->core->lockMgr->reserve(cpu->core, addr);
	return cpu->read<uint32_t>(addr, &cpu->core->regs[instr->rD()]);
}

bool PPCInstr_stwcx(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t base = instr->rA() ? cpu->core->regs[instr->rA()] : 0;
	uint32_t addr = base + cpu->core->regs[instr->rB()];
	if (cpu->core->lockMgr->isReserved(cpu->core, addr)) {
		if (!cpu->write<uint32_t>(addr, cpu->core->regs[instr->rS()])) return false;
		cpu->core->cr.set(PPCCore::EQ, true);
		cpu->core->lockMgr->reset();
	}
	else {
		cpu->core->cr.set(PPCCore::EQ, false);
	}
	cpu->core->cr.set(PPCCore::LT | PPCCore::GT, false);
	return true;
}

bool PPCInstr_crxor(PPCInstruction *instr, PPCInterpreter *cpu) {
	cpu->core->cr.set(1 << instr->crbD(),
		cpu->core->cr.get(1 << instr->crbA()) ^ cpu->core->cr.get(1 << instr->crbB())
	);
	return true;
}

bool PPCInstr_mfcr(PPCInstruction *instr, PPCInterpreter *cpu) {
	cpu->core->regs[instr->rD()] = cpu->core->cr;
	return true;
}

bool PPCInstr_mtcrf(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t mask = 0;
	for (int i = 7; i >= 0; i--) {
		mask <<= 4;
		if (instr->crm() & (1 << i)) {
			mask |= 0xF;
		}
	}
	
	uint32_t value = cpu->core->regs[instr->rS()] & mask;
	cpu->core->cr.value = (cpu->core->cr.value & ~mask) | value;
	return true;
}

bool PPCInstr_mftb(PPCInstruction *instr, PPCInterpreter *cpu) {
	return cpu->core->getSpr((PPCCore::SPR)instr->spr(), &cpu->core->regs[instr->rD()]);
}

bool PPCInstr_mfspr(PPCInstruction *instr, PPCInterpreter *cpu) {
	return cpu->core->getSpr((PPCCore::SPR)instr->spr(), &cpu->core->regs[instr->rD()]);
}

bool PPCInstr_mtspr(PPCInstruction *instr, PPCInterpreter *cpu) {
	return cpu->core->setSpr((PPCCore::SPR)instr->spr(), cpu->core->regs[instr->rS()]);
}

bool PPCInstr_sc(PPCInstruction *instr, PPCInterpreter *cpu) {
	return cpu->core->triggerException(PPCCore::SystemCall);
}

/********** SUPERVISOR INSTRUCTIONS **********/

bool PPCInstr_mfmsr(PPCInstruction *instr, PPCInterpreter *cpu) {
	cpu->core->regs[instr->rD()] = cpu->core->msr;
	return true;
}

bool PPCInstr_mtmsr(PPCInstruction *instr, PPCInterpreter *cpu) {
	return cpu->core->setMsr(cpu->core->regs[instr->rS()]);
}

bool PPCInstr_mfsr(PPCInstruction *instr, PPCInterpreter *cpu) {
	return cpu->core->getSr(instr->sr(), &cpu->core->regs[instr->rD()]);
}

bool PPCInstr_mtsr(PPCInstruction *instr, PPCInterpreter *cpu) {
	return cpu->core->setSr(instr->sr(), cpu->core->regs[instr->rS()]);
}

bool PPCInstr_tlbie(PPCInstruction *instr, PPCInterpreter *cpu) {
	cpu->invalidateMMUCache();
	return true;
}

bool PPCInstr_rfi(PPCInstruction *instr, PPCInterpreter *cpu) {
	cpu->core->pc = cpu->core->srr0;
	if (!cpu->core->setMsr(cpu->core->srr1)) return false;
	return true;
}

/********** CACHE AND SYNCHRONIZATION INSTRUCTIONS **********/

bool PPCInstr_sync(PPCInstruction *instr, PPCInterpreter *cpu) { return true; }
bool PPCInstr_isync(PPCInstruction *instr, PPCInterpreter *cpu) { return true; }
bool PPCInstr_eieio(PPCInstruction *instr, PPCInterpreter *cpu) { return true; }
bool PPCInstr_dcbf(PPCInstruction *instr, PPCInterpreter *cpu) { return true; }
bool PPCInstr_dcbi(PPCInstruction *instr, PPCInterpreter *cpu) { return true; }
bool PPCInstr_dcbz(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t base = instr->rA() ? cpu->core->regs[instr->rA()] : 0;
	uint32_t addr = (base + cpu->core->regs[instr->rB()]) & ~0x1F;
	for (int i = 0; i < 4; i++) {
		if (!cpu->write<uint64_t>(addr, 0)) return false;
		addr += 8;
	}
	return true;
}
bool PPCInstr_dcbz_l(PPCInstruction *instr, PPCInterpreter *cpu) { return true; }
bool PPCInstr_dcbst(PPCInstruction *instr, PPCInterpreter *cpu) { return true; }
bool PPCInstr_icbi(PPCInstruction *instr, PPCInterpreter *cpu) {
	cpu->invalidateICache();
	return true;
}

/********** FLOATING POINT INSTRUCTIONS **********/

bool PPCInstr_fmr(PPCInstruction *instr, PPCInterpreter *cpu) {
	cpu->core->fprs[instr->rD()].ps0 = cpu->core->fprs[instr->rB()].ps0;
	return true;
}

bool PPCInstr_frsp(PPCInstruction *instr, PPCInterpreter *cpu) {
	cpu->core->fprs[instr->rD()].ps0 = (float)cpu->core->fprs[instr->rB()].dbl;
	return true;
}

bool PPCInstr_fctiwz(PPCInstruction *instr, PPCInterpreter *cpu) {
	cpu->core->fprs[instr->rD()].iw1 = (int32_t)cpu->core->fprs[instr->rB()].ps0;
	return true;
}

bool PPCInstr_fneg(PPCInstruction *instr, PPCInterpreter *cpu) {
	cpu->core->fprs[instr->rD()].dbl = -cpu->core->fprs[instr->rB()].dbl;
	return true;
}

bool PPCInstr_fabs(PPCInstruction *instr, PPCInterpreter *cpu) {
	float value = cpu->core->fprs[instr->rB()].ps0;
	if (value < 0) value = -value;
	cpu->core->fprs[instr->rD()].ps0 = value;
	return true;
}

bool PPCInstr_fmuls(PPCInstruction *instr, PPCInterpreter *cpu) {
	cpu->core->fprs[instr->rD()].ps0 = cpu->core->fprs[instr->rA()].ps0 * cpu->core->fprs[instr->rC()].ps0;
	return true;
}

bool PPCInstr_fmul(PPCInstruction *instr, PPCInterpreter *cpu) {
	cpu->core->fprs[instr->rD()].dbl = cpu->core->fprs[instr->rA()].dbl * cpu->core->fprs[instr->rC()].dbl;
	return true;
}

bool PPCInstr_fmadds(PPCInstruction *instr, PPCInterpreter *cpu) {
	cpu->core->fprs[instr->rD()].ps0 = cpu->core->fprs[instr->rA()].ps0 *
		cpu->core->fprs[instr->rC()].ps0 + cpu->core->fprs[instr->rB()].ps0;
	return true;
}

bool PPCInstr_fmadd(PPCInstruction *instr, PPCInterpreter *cpu) {
	cpu->core->fprs[instr->rD()].dbl = cpu->core->fprs[instr->rA()].dbl *
		cpu->core->fprs[instr->rC()].dbl + cpu->core->fprs[instr->rB()].dbl;
	return true;
}

bool PPCInstr_fmsubs(PPCInstruction *instr, PPCInterpreter *cpu) {
	cpu->core->fprs[instr->rD()].ps0 = cpu->core->fprs[instr->rA()].ps0 * 
		cpu->core->fprs[instr->rC()].ps0 - cpu->core->fprs[instr->rB()].ps0;
	return true;
}

bool PPCInstr_fdivs(PPCInstruction *instr, PPCInterpreter *cpu) {
	cpu->core->fprs[instr->rD()].ps0 = cpu->core->fprs[instr->rA()].ps0 / cpu->core->fprs[instr->rB()].ps0;
	return true;
}

bool PPCInstr_fdiv(PPCInstruction *instr, PPCInterpreter *cpu) {
	cpu->core->fprs[instr->rD()].dbl = cpu->core->fprs[instr->rA()].dbl / cpu->core->fprs[instr->rB()].dbl;
	return true;
}

bool PPCInstr_fadds(PPCInstruction *instr, PPCInterpreter *cpu) {
	cpu->core->fprs[instr->rD()].ps0 = cpu->core->fprs[instr->rA()].ps0 + cpu->core->fprs[instr->rB()].ps0;
	return true;
}

bool PPCInstr_fadd(PPCInstruction *instr, PPCInterpreter *cpu) {
	cpu->core->fprs[instr->rD()].dbl = cpu->core->fprs[instr->rA()].dbl + cpu->core->fprs[instr->rB()].dbl;
	return true;
}

bool PPCInstr_fsubs(PPCInstruction *instr, PPCInterpreter *cpu) {
	cpu->core->fprs[instr->rD()].ps0 = cpu->core->fprs[instr->rA()].ps0 - cpu->core->fprs[instr->rB()].ps0;
	return true;
}

bool PPCInstr_fsub(PPCInstruction *instr, PPCInterpreter *cpu) {
	cpu->core->fprs[instr->rD()].dbl = cpu->core->fprs[instr->rA()].dbl - cpu->core->fprs[instr->rB()].dbl;
	return true;
}

bool PPCInstr_fsel(PPCInstruction *instr, PPCInterpreter *cpu) {
	if (cpu->core->fprs[instr->rA()].dbl >= 0) {
		cpu->core->fprs[instr->rD()].dbl = cpu->core->fprs[instr->rC()].dbl;
	}
	else {
		cpu->core->fprs[instr->rD()].dbl = cpu->core->fprs[instr->rB()].dbl;
	}
	return true;
}

bool PPCInstr_fcmpu(PPCInstruction *instr, PPCInterpreter *cpu) {
	float left = cpu->core->fprs[instr->rA()].ps0;
	float right = cpu->core->fprs[instr->rB()].ps1;
	cpu->core->cr.set(PPCCore::LT >> (4 * instr->crfD()), left < right);
	cpu->core->cr.set(PPCCore::GT >> (4 * instr->crfD()), left > right);
	cpu->core->cr.set(PPCCore::EQ >> (4 * instr->crfD()), left == right);
	return true;
}

/********** PAIRED SINGLE INSTRUCTIONS **********/

template <class T>
bool PSQ_LoadTmpl(PPCInterpreter *cpu, uint32_t addr, int rD, int scale, int w) {
	NotImplementedError("PSQ_LoadTmpl");
	return false;
}

template <class T>
bool PSQ_StoreTmpl(PPCInterpreter *cpu, uint32_t addr, int rD, int scale, int w) {
	NotImplementedError("PSQ_StoreTmpl");
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

bool PSQ_Store(PPCInterpreter *cpu, uint32_t addr, int rS, int i, int w) {
	uint32_t config = cpu->core->gqrs[i];
	int scale = (config >> 8) & 0x3F;
	int type = config & 7;
	if (type & 4) { //Quantize
		if (type & 2) { //Signed
			if (type & 1) return PSQ_StoreTmpl<int16_t>(cpu, addr, rS, scale, w);
			return PSQ_StoreTmpl<int8_t>(cpu, addr, rS, scale, w);
		}
		if (type & 1) return PSQ_StoreTmpl<uint16_t>(cpu, addr, rS, scale, w);
		return PSQ_StoreTmpl<uint8_t>(cpu, addr, rS, scale, w);
	}
	
	if (w) {
		return cpu->write<float>(addr, cpu->core->fprs[i].ps0);
	}
	else {
		if (!cpu->write<float>(addr, cpu->core->fprs[i].ps0)) return false;
		return cpu->write<float>(addr+4, cpu->core->fprs[i].ps1);
	}
}

bool PPCInstr_psq_l(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t addr = (instr->rA() ? cpu->core->regs[instr->rA()] : 0) + instr->ps_d();
	return PSQ_Load(cpu, addr, instr->rD(), instr->ps_i(), instr->ps_w());
}

bool PPCInstr_psq_st(PPCInstruction *instr, PPCInterpreter *cpu) {
	uint32_t addr = (instr->rA() ? cpu->core->regs[instr->rA()] : 0) + instr->ps_d();
	return PSQ_Store(cpu, addr, instr->rS(), instr->ps_i(), instr->ps_w());
}

bool PPCInstr_ps_merge00(PPCInstruction *instr, PPCInterpreter *cpu) {
	cpu->core->fprs[instr->rD()].ps0 = cpu->core->fprs[instr->rA()].ps0;
	cpu->core->fprs[instr->rD()].ps1 = cpu->core->fprs[instr->rB()].ps0;
	return true;
}

bool PPCInstr_ps_merge01(PPCInstruction *instr, PPCInterpreter *cpu) {
	cpu->core->fprs[instr->rD()].ps0 = cpu->core->fprs[instr->rA()].ps0;
	cpu->core->fprs[instr->rD()].ps1 = cpu->core->fprs[instr->rB()].ps1;
	return true;
}

bool PPCInstr_ps_merge10(PPCInstruction *instr, PPCInterpreter *cpu) {
	cpu->core->fprs[instr->rD()].ps0 = cpu->core->fprs[instr->rA()].ps1;
	cpu->core->fprs[instr->rD()].ps1 = cpu->core->fprs[instr->rB()].ps0;
	return true;
}

bool PPCInstr_ps_merge11(PPCInstruction *instr, PPCInterpreter *cpu) {
	cpu->core->fprs[instr->rD()].ps0 = cpu->core->fprs[instr->rA()].ps1;
	cpu->core->fprs[instr->rD()].ps1 = cpu->core->fprs[instr->rB()].ps1;
	return true;
}

bool PPCInstr_ps_sub(PPCInstruction *instr, PPCInterpreter *cpu) {
	cpu->core->fprs[instr->rD()].ps0 = cpu->core->fprs[instr->rA()].ps0 - cpu->core->fprs[instr->rB()].ps0;
	cpu->core->fprs[instr->rD()].ps1 = cpu->core->fprs[instr->rA()].ps1 - cpu->core->fprs[instr->rB()].ps1;
	return true;
}

bool PPCInstruction::execute(PPCInterpreter *cpu) {
	switch(opcode()) {
		case 4:
			switch(opcode2()) {
				case 20: return PPCInstr_ps_sub(this, cpu);
				case 528: return PPCInstr_ps_merge00(this, cpu);
				case 560: return PPCInstr_ps_merge01(this, cpu);
				case 592: return PPCInstr_ps_merge10(this, cpu);
				case 624: return PPCInstr_ps_merge11(this, cpu);
				case 1014: return PPCInstr_dcbz_l(this, cpu);
				default:
					NotImplementedError("PPC opcode 4: %i", opcode2());
					return false;
			}
		case 7: return PPCInstr_mulli(this, cpu);
		case 8: return PPCInstr_subfic(this, cpu);
		case 10: return PPCInstr_cmpli(this, cpu);
		case 11: return PPCInstr_cmpi(this, cpu);
		case 12: return PPCInstr_addic(this, cpu);
		case 13: return PPCInstr_addic_rc(this, cpu);
		case 14: return PPCInstr_addi(this, cpu);
		case 15: return PPCInstr_addis(this, cpu);
		case 16: return PPCInstr_bc(this, cpu);
		case 17: return PPCInstr_sc(this, cpu);
		case 18: return PPCInstr_b(this, cpu);
		case 19:
			switch(opcode2()) {
				case 16: return PPCInstr_bclr(this, cpu);
				case 50: return PPCInstr_rfi(this, cpu);
				case 150: return PPCInstr_isync(this, cpu);
				case 193: return PPCInstr_crxor(this, cpu);
				case 528: return PPCInstr_bcctr(this, cpu);
				default:
					NotImplementedError("PPC opcode 19: %i", opcode2());
					return false;
			}
		case 20: return PPCInstr_rlwimi(this, cpu);
		case 21: return PPCInstr_rlwinm(this, cpu);
		case 24: return PPCInstr_ori(this, cpu);
		case 25: return PPCInstr_oris(this, cpu);
		case 26: return PPCInstr_xori(this, cpu);
		case 27: return PPCInstr_xoris(this, cpu);
		case 28: return PPCInstr_andi(this, cpu);
		case 29: return PPCInstr_andis(this, cpu);
		case 31:
			switch(opcode2()) {
				case 0: return PPCInstr_cmp(this, cpu);
				case 8: return PPCInstr_subfc(this, cpu);
				case 10: return PPCInstr_addc(this, cpu);
				case 11: return PPCInstr_mulhwu(this, cpu);
				case 19: return PPCInstr_mfcr(this, cpu);
				case 20: return PPCInstr_lwarx(this, cpu);
				case 23: return PPCInstr_lwzx(this, cpu);
				case 24: return PPCInstr_slw(this, cpu);
				case 26: return PPCInstr_cntlzw(this, cpu);
				case 28: return PPCInstr_and(this, cpu);
				case 32: return PPCInstr_cmpl(this, cpu);
				case 40: return PPCInstr_subf(this, cpu);
				case 54: return PPCInstr_dcbst(this, cpu);
				case 55: return PPCInstr_lwzux(this, cpu);
				case 60: return PPCInstr_andc(this, cpu);
				case 75: return PPCInstr_mulhw(this, cpu);
				case 83: return PPCInstr_mfmsr(this, cpu);
				case 86: return PPCInstr_dcbf(this, cpu);
				case 87: return PPCInstr_lbzx(this, cpu);
				case 104: return PPCInstr_neg(this, cpu);
				case 119: return PPCInstr_lbzux(this, cpu);
				case 124: return PPCInstr_nor(this, cpu);
				case 136: return PPCInstr_subfe(this, cpu);
				case 138: return PPCInstr_adde(this, cpu);
				case 144: return PPCInstr_mtcrf(this, cpu);
				case 146: return PPCInstr_mtmsr(this, cpu);
				case 150: return PPCInstr_stwcx(this, cpu);
				case 151: return PPCInstr_stwx(this, cpu);
				case 183: return PPCInstr_stwux(this, cpu);
				case 200: return PPCInstr_subfze(this, cpu);
				case 202: return PPCInstr_addze(this, cpu);
				case 210: return PPCInstr_mtsr(this, cpu);
				case 215: return PPCInstr_stbx(this, cpu);
				case 235: return PPCInstr_mullw(this, cpu);
				case 247: return PPCInstr_stbux(this, cpu);
				case 266: return PPCInstr_add(this, cpu);
				case 279: return PPCInstr_lhzx(this, cpu);
				case 306: return PPCInstr_tlbie(this, cpu);
				case 316: return PPCInstr_xor(this, cpu);
				case 339: return PPCInstr_mfspr(this, cpu);
				case 371: return PPCInstr_mftb(this, cpu);
				case 407: return PPCInstr_sthx(this, cpu);
				case 439: return PPCInstr_sthux(this, cpu);
				case 444: return PPCInstr_or(this, cpu);
				case 459: return PPCInstr_divwu(this, cpu);
				case 467: return PPCInstr_mtspr(this, cpu);
				case 470: return PPCInstr_dcbi(this, cpu);
				case 491: return PPCInstr_divw(this, cpu);
				case 534: return PPCInstr_lwbrx(this, cpu);
				case 535: return PPCInstr_lfsx(this, cpu);
				case 536: return PPCInstr_srw(this, cpu);
				case 595: return PPCInstr_mfsr(this, cpu);
				case 598: return PPCInstr_sync(this, cpu);
				case 599: return PPCInstr_lfdx(this, cpu);
				case 662: return PPCInstr_stwbrx(this, cpu);
				case 663: return PPCInstr_stfsx(this, cpu);
				case 727: return PPCInstr_stfdx(this, cpu);
				case 792: return PPCInstr_sraw(this, cpu);
				case 824: return PPCInstr_srawi(this, cpu);
				case 854: return PPCInstr_eieio(this, cpu);
				case 922: return PPCInstr_extsh(this, cpu);
				case 954: return PPCInstr_extsb(this, cpu);
				case 982: return PPCInstr_icbi(this, cpu);
				case 983: return PPCInstr_stfiwx(this, cpu);
				case 1014: return PPCInstr_dcbz(this, cpu);
				default:
					NotImplementedError("PPC opcode 31: %i", opcode2());
					return false;
			}
		case 32: return PPCInstr_lwz(this, cpu);
		case 33: return PPCInstr_lwzu(this, cpu);
		case 34: return PPCInstr_lbz(this, cpu);
		case 35: return PPCInstr_lbzu(this, cpu);
		case 36: return PPCInstr_stw(this, cpu);
		case 37: return PPCInstr_stwu(this, cpu);
		case 38: return PPCInstr_stb(this, cpu);
		case 39: return PPCInstr_stbu(this, cpu);
		case 40: return PPCInstr_lhz(this, cpu);
		case 41: return PPCInstr_lhzu(this, cpu);
		case 42: return PPCInstr_lha(this, cpu);
		case 44: return PPCInstr_sth(this, cpu);
		case 45: return PPCInstr_sthu(this, cpu);
		case 46: return PPCInstr_lmw(this, cpu);
		case 47: return PPCInstr_stmw(this, cpu);
		case 48: return PPCInstr_lfs(this, cpu);
		case 49: return PPCInstr_lfsu(this, cpu);
		case 50: return PPCInstr_lfd(this, cpu);
		case 52: return PPCInstr_stfs(this, cpu);
		case 53: return PPCInstr_stfsu(this, cpu);
		case 54: return PPCInstr_stfd(this, cpu);
		case 56: return PPCInstr_psq_l(this, cpu);
		case 59:
			switch(opcode3()) {
				case 18: return PPCInstr_fdivs(this, cpu);
				case 20: return PPCInstr_fsubs(this, cpu);
				case 21: return PPCInstr_fadds(this, cpu);
				case 25: return PPCInstr_fmuls(this, cpu);
				case 28: return PPCInstr_fmsubs(this, cpu);
				case 29: return PPCInstr_fmadds(this, cpu);
				default:
					NotImplementedError("PPC opcode 59: %i", opcode3());
					return false;
			}
		case 60: return PPCInstr_psq_st(this, cpu);
		case 63:
			switch(opcode2()) {
				case 0: return PPCInstr_fcmpu(this, cpu);
				case 12: return PPCInstr_frsp(this, cpu);
				case 15: return PPCInstr_fctiwz(this, cpu);
				case 18: return PPCInstr_fdiv(this, cpu);
				case 20: return PPCInstr_fsub(this, cpu);
				case 21: return PPCInstr_fadd(this, cpu);
				case 40: return PPCInstr_fneg(this, cpu);
				case 72: return PPCInstr_fmr(this, cpu);
				case 264: return PPCInstr_fabs(this, cpu);
				default:
					switch(opcode3()) {
						case 23: return PPCInstr_fsel(this, cpu);
						case 25: return PPCInstr_fmul(this, cpu);
						case 29: return PPCInstr_fmadd(this, cpu);
					}
					NotImplementedError("PPC opcode 63: %i", opcode2());
					return false;
			}
		default:
			NotImplementedError("PPC opcode %i", opcode());
			return false;
	}
}
