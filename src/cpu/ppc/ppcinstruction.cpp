
#include "ppcinstruction.h"
#include "ppcprocessor.h"

#include "common/exceptions.h"

#include <cmath>


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

bool checkCondition(PPCInstruction *instr, PPCProcessor *cpu) {
	int bo = instr->bo();
	if (!(bo & 4)) {
		cpu->core.sprs[PPCCore::CTR]--;
		if ((bo & 2) && cpu->core.sprs[PPCCore::CTR] != 0) return false;
		else if (cpu->core.sprs[PPCCore::CTR] == 0) return false;
	}
	if (bo & 0x10) return true;
	if (bo & 8) return (cpu->core.cr >> (31 - instr->bi())) & 1;
	return !((cpu->core.cr >> (31 - instr->bi())) & 1);
}

void updateConditions(PPCCore *core, uint32_t result) {
	core->cr.set(PPCCore::LT, result >> 31);
	core->cr.set(PPCCore::GT, !(result >> 31) && result != 0);
	core->cr.set(PPCCore::EQ, result == 0);
}

/********** SUPERVISOR INSTRUCTIONS **********/

void PPCInstr_mfmsr(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.regs[instr->rD()] = cpu->core.msr;
}

void PPCInstr_mtmsr(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.msr = cpu->core.regs[instr->rS()];
	cpu->core.checkPendingExceptions();
}

void PPCInstr_mfsr(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.regs[instr->rD()] = cpu->core.sr[instr->sr()];
}

void PPCInstr_mtsr(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.sr[instr->sr()] = cpu->core.regs[instr->rS()];
}

void PPCInstr_tlbie(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->mmu.cache.invalidate();
}

void PPCInstr_rfi(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.pc = cpu->core.sprs[PPCCore::SRR0];
	cpu->core.msr = cpu->core.sprs[PPCCore::SRR1];
	cpu->core.checkPendingExceptions();
}

/********** CACHE AND SYNCHRONIZATION INSTRUCTIONS **********/

void PPCInstr_dcbz(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t base = instr->rA() ? cpu->core.regs[instr->rA()] : 0;
	uint32_t addr = (base + cpu->core.regs[instr->rB()]) & ~0x1F;
	for (int i = 0; i < 4; i++) {
		if (!cpu->write<uint64_t>(addr, 0)) return;
		addr += 8;
	}
}

void PPCInstr_icbi(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t addr = instr->rA() ? cpu->core.regs[instr->rA()] : 0;
	addr += cpu->core.regs[instr->rB()];
	
	bool supervisor = !(cpu->core.msr & 0x4000);
	if (cpu->mmu.translate(&addr, MemoryAccess::DataRead, supervisor)) {
		cpu->jit.invalidateBlock(addr);
	}
	else {
		cpu->core.sprs[PPCCore::DAR] = addr;
		cpu->core.sprs[PPCCore::DSISR] = 0x40000000;
		cpu->core.triggerException(PPCCore::DSI);
	}
}

/********** NORMAL INSTRUCTIONS **********/

void PPCInstr_mulli(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.regs[instr->rD()] = cpu->core.regs[instr->rA()] * instr->simm();
}

void PPCInstr_addic(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t left = cpu->core.regs[instr->rA()];
	uint32_t right = instr->simm();
	uint64_t result = (uint64_t)left + right;
	if (result >> 32) {
		cpu->core.sprs[PPCCore::XER] |= PPCCore::CA;
	}
	else {
		cpu->core.sprs[PPCCore::XER] &= ~PPCCore::CA;
	}
	cpu->core.regs[instr->rD()] = result;
}

void PPCInstr_addic_rc(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t left = cpu->core.regs[instr->rA()];
	uint32_t right = instr->simm();
	uint64_t result = (uint64_t)left + right;
	if (result >> 32) {
		cpu->core.sprs[PPCCore::XER] |= PPCCore::CA;
	}
	else {
		cpu->core.sprs[PPCCore::XER] &= ~PPCCore::CA;
	}
	updateConditions(&cpu->core, result);
	cpu->core.regs[instr->rD()] = result;
}

void PPCInstr_subfic(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t left = cpu->core.regs[instr->rA()];
	uint32_t right = instr->simm();
	uint64_t result = (uint64_t)(uint32_t)~left + right + 1;
	cpu->core.setCarry(result >> 32);
	cpu->core.regs[instr->rD()] = (uint32_t)result;
}

void PPCInstr_srawi(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t value = cpu->core.regs[instr->rS()];
	uint32_t result = (int32_t)value >> instr->sh();

	if (instr->sh() && (value >> 31) && (value & genmask(32 - instr->sh(), 31))) {
		cpu->core.sprs[PPCCore::XER] |= PPCCore::CA;
	}
	else {
		cpu->core.sprs[PPCCore::XER] &= ~PPCCore::CA;
	}

	if (instr->rc()) updateConditions(&cpu->core, result);
	cpu->core.regs[instr->rA()] = result;
}

void PPCInstr_add(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t result = cpu->core.regs[instr->rA()] + cpu->core.regs[instr->rB()];
	if (instr->rc()) updateConditions(&cpu->core, result);
	cpu->core.regs[instr->rD()] = result;
}

void PPCInstr_subf(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t result = cpu->core.regs[instr->rB()] - cpu->core.regs[instr->rA()];
	if (instr->rc()) updateConditions(&cpu->core, result);
	cpu->core.regs[instr->rD()] = result;
}

void PPCInstr_mullw(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t result = cpu->core.regs[instr->rA()] * cpu->core.regs[instr->rB()];
	if (instr->rc()) updateConditions(&cpu->core, result);
	cpu->core.regs[instr->rD()] = result;
}


void PPCInstr_mulhw(PPCInstruction *instr, PPCProcessor *cpu) {
	int32_t a = cpu->core.regs[instr->rA()];
	int32_t b = cpu->core.regs[instr->rB()];
	uint64_t result = (int64_t)a * (int64_t)b;
	if (instr->rc()) updateConditions(&cpu->core, result >> 32);
	cpu->core.regs[instr->rD()] = result >> 32;
}

void PPCInstr_mulhwu(PPCInstruction *instr, PPCProcessor *cpu) {
	uint64_t result = (uint64_t)cpu->core.regs[instr->rA()] * cpu->core.regs[instr->rB()];
	if (instr->rc()) updateConditions(&cpu->core, result >> 32);
	cpu->core.regs[instr->rD()] = result >> 32;
}

void PPCInstr_divwu(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t dividend = cpu->core.regs[instr->rA()];
	uint32_t divisor = cpu->core.regs[instr->rB()];
	if (divisor != 0) {
		uint32_t result = dividend / divisor;
		if (instr->rc()) updateConditions(&cpu->core, result);
		cpu->core.regs[instr->rD()] = result;
	}
}

void PPCInstr_divw(PPCInstruction *instr, PPCProcessor *cpu) {
	int32_t dividend = cpu->core.regs[instr->rA()];
	int32_t divisor = cpu->core.regs[instr->rB()];
	if (divisor != 0) {
		int32_t result = dividend / divisor;
		if (instr->rc()) updateConditions(&cpu->core, result);
		cpu->core.regs[instr->rD()] = result;
	}
}

void PPCInstr_neg(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t result = -cpu->core.regs[instr->rA()];
	if (instr->rc()) updateConditions(&cpu->core, result);
	cpu->core.regs[instr->rD()] = result;
}

void PPCInstr_extsb(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t result = (int8_t)cpu->core.regs[instr->rS()];
	if (instr->rc()) updateConditions(&cpu->core, result);
	cpu->core.regs[instr->rA()] = result;
}

void PPCInstr_extsh(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t result = (int16_t)cpu->core.regs[instr->rS()];
	if (instr->rc()) updateConditions(&cpu->core, result);
	cpu->core.regs[instr->rA()] = result;
}

void PPCInstr_cntlzw(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t result = cntlzw(cpu->core.regs[instr->rS()]);
	if (instr->rc()) updateConditions(&cpu->core, result);
	cpu->core.regs[instr->rA()] = result;
}

void PPCInstr_or(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t result = cpu->core.regs[instr->rS()] | cpu->core.regs[instr->rB()];
	if (instr->rc()) updateConditions(&cpu->core, result);
	cpu->core.regs[instr->rA()] = result;
}

void PPCInstr_and(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t result = cpu->core.regs[instr->rS()] & cpu->core.regs[instr->rB()];
	if (instr->rc()) updateConditions(&cpu->core, result);
	cpu->core.regs[instr->rA()] = result;
}

void PPCInstr_xor(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t result = cpu->core.regs[instr->rS()] ^ cpu->core.regs[instr->rB()];
	if (instr->rc()) updateConditions(&cpu->core, result);
	cpu->core.regs[instr->rA()] = result;
}

void PPCInstr_nor(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t result = ~(cpu->core.regs[instr->rS()] | cpu->core.regs[instr->rB()]);
	if (instr->rc()) updateConditions(&cpu->core, result);
	cpu->core.regs[instr->rA()] = result;
}

void PPCInstr_andc(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t result = cpu->core.regs[instr->rS()] & ~cpu->core.regs[instr->rB()];
	if (instr->rc()) updateConditions(&cpu->core, result);
	cpu->core.regs[instr->rA()] = result;
}

void PPCInstr_slw(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t bits = cpu->core.regs[instr->rB()] & 0x3F;
	uint32_t result = 0;
	if (!(bits & 0x20)) {
		result = cpu->core.regs[instr->rS()] << bits;
	}
	if (instr->rc()) updateConditions(&cpu->core, result);
	cpu->core.regs[instr->rA()] = result;
}

void PPCInstr_srw(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t bits = cpu->core.regs[instr->rB()] & 0x3F;
	uint32_t result = 0;
	if (!(bits & 0x20)) {
		result = cpu->core.regs[instr->rS()] >> bits;
	}
	if (instr->rc()) updateConditions(&cpu->core, result);
	cpu->core.regs[instr->rA()] = result;
}

void PPCInstr_sraw(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t value = cpu->core.regs[instr->rS()];
	uint32_t bits = cpu->core.regs[instr->rB()] & 0x3F;
	
	uint32_t result;
	if (!(bits & 0x20)) {
		result = (int32_t)value >> bits;
		if (bits && (value >> 31) && (value & genmask(32 - bits, 31))) {
			cpu->core.sprs[PPCCore::XER] |= PPCCore::CA;
		}
		else {
			cpu->core.sprs[PPCCore::XER] &= ~PPCCore::CA;
		}
	}
	else {
		result = (int32_t)value >> 31;
		if (value >> 31) {
			cpu->core.sprs[PPCCore::XER] |= PPCCore::CA;
		}
		else {
			cpu->core.sprs[PPCCore::XER] &= ~PPCCore::CA;
		}
	}
	
	if (instr->rc()) updateConditions(&cpu->core, result);
	cpu->core.regs[instr->rA()] = result;
}

void PPCInstr_addc(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t left = cpu->core.regs[instr->rA()];
	uint32_t right = cpu->core.regs[instr->rB()];
	uint64_t result = (uint64_t)left + right;
	cpu->core.setCarry(result >> 32);
	if (instr->rc()) updateConditions(&cpu->core, (uint32_t)result);
	cpu->core.regs[instr->rD()] = (uint32_t)result;
}

void PPCInstr_subfc(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t left = cpu->core.regs[instr->rA()];
	uint32_t right = cpu->core.regs[instr->rB()];
	uint64_t result = (uint64_t)(uint32_t)~left + right + 1;
	cpu->core.setCarry(result >> 32);
	if (instr->rc()) updateConditions(&cpu->core, (uint32_t)result);
	cpu->core.regs[instr->rD()] = (uint32_t)result;
}

void PPCInstr_adde(PPCInstruction *instr, PPCProcessor *cpu) {
	uint64_t result = (uint64_t)cpu->core.regs[instr->rA()] + cpu->core.regs[instr->rB()] + cpu->core.getCarry();
	cpu->core.setCarry(result >> 32);
	if (instr->rc()) updateConditions(&cpu->core, (uint32_t)result);
	cpu->core.regs[instr->rD()] = (uint32_t)result;
}

void PPCInstr_addze(PPCInstruction *instr, PPCProcessor *cpu) {
	uint64_t result = (uint64_t)cpu->core.regs[instr->rA()] + cpu->core.getCarry();
	cpu->core.setCarry(result >> 32);
	if (instr->rc()) updateConditions(&cpu->core, (uint32_t)result);
	cpu->core.regs[instr->rD()] = (uint32_t)result;
}

void PPCInstr_subfe(PPCInstruction *instr, PPCProcessor *cpu) {
	uint64_t result = (uint64_t)(uint32_t)~cpu->core.regs[instr->rA()] + cpu->core.regs[instr->rB()] + cpu->core.getCarry();
	cpu->core.setCarry(result >> 32);
	if (instr->rc()) updateConditions(&cpu->core, (uint32_t)result);
	cpu->core.regs[instr->rD()] = (uint32_t)result;
}

void PPCInstr_subfze(PPCInstruction *instr, PPCProcessor *cpu) {
	uint64_t result = (uint64_t)(uint32_t)~cpu->core.regs[instr->rA()] + cpu->core.getCarry();
	cpu->core.setCarry(result >> 32);
	if (instr->rc()) updateConditions(&cpu->core, (uint32_t)result);
	cpu->core.regs[instr->rD()] = (uint32_t)result;
}

void PPCInstr_cmpli(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t left = cpu->core.regs[instr->rA()];
	uint32_t right = instr->uimm();
	cpu->core.cr.set(PPCCore::LT >> (4 * instr->crfD()), left < right);
	cpu->core.cr.set(PPCCore::GT >> (4 * instr->crfD()), left > right);
	cpu->core.cr.set(PPCCore::EQ >> (4 * instr->crfD()), left == right);
}

void PPCInstr_cmpi(PPCInstruction *instr, PPCProcessor *cpu) {
	int32_t left = cpu->core.regs[instr->rA()];
	int32_t right = instr->simm();
	cpu->core.cr.set(PPCCore::LT >> (4 * instr->crfD()), left < right);
	cpu->core.cr.set(PPCCore::GT >> (4 * instr->crfD()), left > right);
	cpu->core.cr.set(PPCCore::EQ >> (4 * instr->crfD()), left == right);
}

void PPCInstr_cmpl(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t left = cpu->core.regs[instr->rA()];
	uint32_t right = cpu->core.regs[instr->rB()];
	cpu->core.cr.set(PPCCore::LT >> (4 * instr->crfD()), left < right);
	cpu->core.cr.set(PPCCore::GT >> (4 * instr->crfD()), left > right);
	cpu->core.cr.set(PPCCore::EQ >> (4 * instr->crfD()), left == right);
}

void PPCInstr_cmp(PPCInstruction *instr, PPCProcessor *cpu) {
	int32_t left = cpu->core.regs[instr->rA()];
	int32_t right = cpu->core.regs[instr->rB()];
	cpu->core.cr.set(PPCCore::LT >> (4 * instr->crfD()), left < right);
	cpu->core.cr.set(PPCCore::GT >> (4 * instr->crfD()), left > right);
	cpu->core.cr.set(PPCCore::EQ >> (4 * instr->crfD()), left == right);
}

void PPCInstr_crnor(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.cr.set(1 << instr->crbD(),
		!cpu->core.cr.get(1 << instr->crbA()) && !cpu->core.cr.get(1 << instr->crbB())
	);
}

void PPCInstr_crandc(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.cr.set(1 << instr->crbD(),
		cpu->core.cr.get(1 << instr->crbA()) && !cpu->core.cr.get(1 << instr->crbB())
	);
}

void PPCInstr_crxor(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.cr.set(1 << instr->crbD(),
		cpu->core.cr.get(1 << instr->crbA()) != cpu->core.cr.get(1 << instr->crbB())
	);
}

void PPCInstr_crnand(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.cr.set(1 << instr->crbD(),
		!cpu->core.cr.get(1 << instr->crbA()) || !cpu->core.cr.get(1 << instr->crbB())
	);
}

void PPCInstr_crand(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.cr.set(1 << instr->crbD(),
		cpu->core.cr.get(1 << instr->crbA()) && cpu->core.cr.get(1 << instr->crbB())
	);
}

void PPCInstr_creqv(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.cr.set(1 << instr->crbD(),
		cpu->core.cr.get(1 << instr->crbA()) == cpu->core.cr.get(1 << instr->crbB())
	);
}

void PPCInstr_crorc(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.cr.set(1 << instr->crbD(),
		cpu->core.cr.get(1 << instr->crbA()) || !cpu->core.cr.get(1 << instr->crbB())
	);
}

void PPCInstr_cror(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.cr.set(1 << instr->crbD(),
		cpu->core.cr.get(1 << instr->crbA()) || cpu->core.cr.get(1 << instr->crbB())
	);
}

void PPCInstr_mfcr(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.regs[instr->rD()] = cpu->core.cr;
}

void PPCInstr_mtcrf(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t mask = 0;
	for (int i = 7; i >= 0; i--) {
		mask <<= 4;
		if (instr->crm() & (1 << i)) {
			mask |= 0xF;
		}
	}
	
	uint32_t value = cpu->core.regs[instr->rS()] & mask;
	cpu->core.cr = (cpu->core.cr & ~mask) | value;
}

void PPCInstr_bclr(PPCInstruction *instr, PPCProcessor *cpu) {
	if (checkCondition(instr, cpu)) {
		uint32_t target = cpu->core.sprs[PPCCore::LR];
		if (instr->lk()) {
			cpu->core.sprs[PPCCore::LR] = cpu->core.pc;
		}
		cpu->core.pc = target;
	}
}

void PPCInstr_bcctr(PPCInstruction *instr, PPCProcessor *cpu) {
	if (checkCondition(instr, cpu)) {
		if (instr->lk()) {
			cpu->core.sprs[PPCCore::LR] = cpu->core.pc;
		}
		cpu->core.pc = cpu->core.sprs[PPCCore::CTR];
	}
}

PPCCore::SPR convertSpr(int spr) {
	if (spr == 0x19) return PPCCore::SDR1;
	spr &= ~0x10;
	return (PPCCore::SPR)((spr & 0xF) | ((spr >> 1) & ~0xF));
}

void PPCInstr_mftb(PPCInstruction *instr, PPCProcessor *cpu) {
	PPCCore::SPR spr = convertSpr(instr->spr());
	cpu->core.regs[instr->rD()] = cpu->core.sprs[spr];
}

void PPCInstr_mfspr(PPCInstruction *instr, PPCProcessor *cpu) {
	PPCCore::SPR spr = convertSpr(instr->spr());
	cpu->core.regs[instr->rD()] = cpu->core.sprs[spr];
}

void PPCInstr_mtspr(PPCInstruction *instr, PPCProcessor *cpu) {
	PPCCore::SPR spr = convertSpr(instr->spr());
	cpu->core.sprs[spr] = cpu->core.regs[instr->rS()];
}

void PPCInstr_sc(PPCInstruction *instr, PPCProcessor *cpu) {
	return cpu->core.triggerException(PPCCore::SystemCall);
}

template <class T>
void PPCInstr_loadu(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t addr = cpu->core.regs[instr->rA()] + instr->d();
	
	T value;
	if (cpu->read<T>(addr, &value)) {
		cpu->core.regs[instr->rA()] = addr;
		cpu->core.regs[instr->rD()] = value;
	}
}

template <class T>
void PPCInstr_loadx(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t base = instr->rA() ? cpu->core.regs[instr->rA()] : 0;
	uint32_t addr = base + cpu->core.regs[instr->rB()];
	
	T value;
	if (cpu->read<T>(addr, &value)) {
		cpu->core.regs[instr->rD()] = value;
	}
}

template <class T>
void PPCInstr_loadux(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t addr = cpu->core.regs[instr->rA()] + cpu->core.regs[instr->rB()];
	
	T value;
	if (cpu->read<T>(addr, &value)) {
		cpu->core.regs[instr->rA()] = addr;
		cpu->core.regs[instr->rD()] = value;
	}
}

template <class T>
void PPCInstr_store(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t addr = (instr->rA() ? cpu->core.regs[instr->rA()] : 0) + instr->d();
	cpu->write<T>(addr, cpu->core.regs[instr->rS()]);
}

template <class T>
void PPCInstr_storeu(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t addr = cpu->core.regs[instr->rA()] + instr->d();
	if (cpu->write<T>(addr, cpu->core.regs[instr->rS()])) {
		cpu->core.regs[instr->rA()] = addr;
	}
}

template <class T>
void PPCInstr_storex(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t base = instr->rA() ? cpu->core.regs[instr->rA()] : 0;
	uint32_t addr = base + cpu->core.regs[instr->rB()];
	cpu->write<T>(addr, cpu->core.regs[instr->rS()]);
}

template <class T>
void PPCInstr_storeux(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t addr = cpu->core.regs[instr->rA()] + cpu->core.regs[instr->rB()];
	if (cpu->write<T>(addr, cpu->core.regs[instr->rS()])) {
		cpu->core.regs[instr->rA()] = addr;
	}
}

void PPCInstr_lmw(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t addr = (instr->rA() ? cpu->core.regs[instr->rA()] : 0) + instr->d();
	for (int reg = instr->rD(); reg < 32; reg++) {
		if (!cpu->read<uint32_t>(addr, &cpu->core.regs[reg])) return;
		addr += 4;
	}
}

void PPCInstr_stmw(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t addr = (instr->rA() ? cpu->core.regs[instr->rA()] : 0) + instr->d();
	for (int reg = instr->rS(); reg < 32; reg++) {
		if (!cpu->write<uint32_t>(addr, cpu->core.regs[reg])) return;
		addr += 4;
	}
}

void PPCInstr_lwarx(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t base = instr->rA() ? cpu->core.regs[instr->rA()] : 0;
	uint32_t addr = base + cpu->core.regs[instr->rB()];
	cpu->reservation->reserve(cpu, addr);
	cpu->read<uint32_t>(addr, &cpu->core.regs[instr->rD()]);
}

void PPCInstr_stwcx(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t base = instr->rA() ? cpu->core.regs[instr->rA()] : 0;
	uint32_t addr = base + cpu->core.regs[instr->rB()];
	if (cpu->reservation->check(cpu, addr)) {
		if (!cpu->write<uint32_t>(addr, cpu->core.regs[instr->rS()])) return;
		cpu->core.cr.set(PPCCore::EQ, true);
		cpu->reservation->reset();
	}
	else {
		cpu->core.cr.set(PPCCore::EQ, false);
	}
	cpu->core.cr.set(PPCCore::LT | PPCCore::GT, false);
}

/********** FLOATING POINT INSTRUCTIONS **********/

void PPCInstr_lfs(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t addr = (instr->rA() ? cpu->core.regs[instr->rA()] : 0) + instr->d();
	cpu->read<float>(addr, &cpu->core.fprs[instr->rD()].ps0);
}

void PPCInstr_lfd(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t addr = (instr->rA() ? cpu->core.regs[instr->rA()] : 0) + instr->d();
	cpu->read<double>(addr, &cpu->core.fprs[instr->rD()].dbl);
}

void PPCInstr_lfsu(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t addr = cpu->core.regs[instr->rA()] + instr->d();
	if (cpu->read<float>(addr, &cpu->core.fprs[instr->rD()].ps0)) {
		cpu->core.regs[instr->rA()] = addr;
	}
}

void PPCInstr_lfdu(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t addr = cpu->core.regs[instr->rA()] + instr->d();
	if (cpu->read<double>(addr, &cpu->core.fprs[instr->rD()].dbl)) {
		cpu->core.regs[instr->rA()] = addr;
	}
}

void PPCInstr_lfsx(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t addr = cpu->core.regs[instr->rA()] + cpu->core.regs[instr->rB()];
	cpu->read<float>(addr, &cpu->core.fprs[instr->rD()].ps0);
}

void PPCInstr_lfdx(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t addr = cpu->core.regs[instr->rA()] + cpu->core.regs[instr->rB()];
	cpu->read<double>(addr, &cpu->core.fprs[instr->rD()].dbl);
}

void PPCInstr_lfsux(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t addr = cpu->core.regs[instr->rA()] + cpu->core.regs[instr->rB()];
	if (cpu->read<float>(addr, &cpu->core.fprs[instr->rD()].ps0)) {
		cpu->core.regs[instr->rA()] = addr;
	}
}

void PPCInstr_lfdux(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t addr = cpu->core.regs[instr->rA()] + cpu->core.regs[instr->rB()];
	if (cpu->read<double>(addr, &cpu->core.fprs[instr->rD()].dbl)) {
		cpu->core.regs[instr->rA()] = addr;
	}
}

void PPCInstr_stfs(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t addr = (instr->rA() ? cpu->core.regs[instr->rA()] : 0) + instr->d();
	cpu->write<float>(addr, cpu->core.fprs[instr->rS()].ps0);
}

void PPCInstr_stfd(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t addr = (instr->rA() ? cpu->core.regs[instr->rA()] : 0) + instr->d();
	cpu->write<double>(addr, cpu->core.fprs[instr->rS()].dbl);
}

void PPCInstr_stfsu(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t addr = cpu->core.regs[instr->rA()] + instr->d();
	if (cpu->write<float>(addr, cpu->core.fprs[instr->rS()].ps0)) {
		cpu->core.regs[instr->rA()] = addr;
	}
}

void PPCInstr_stfdu(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t addr = cpu->core.regs[instr->rA()] + instr->d();
	if (cpu->write<double>(addr, cpu->core.fprs[instr->rS()].dbl)) {
		cpu->core.regs[instr->rA()] = addr;
	}
}

void PPCInstr_stfsx(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t addr = cpu->core.regs[instr->rA()] + cpu->core.regs[instr->rB()];
	cpu->write<float>(addr, cpu->core.fprs[instr->rS()].ps0);
}

void PPCInstr_stfdx(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t addr = cpu->core.regs[instr->rA()] + cpu->core.regs[instr->rB()];
	cpu->write<double>(addr, cpu->core.fprs[instr->rS()].dbl);
}

void PPCInstr_stfsux(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t addr = cpu->core.regs[instr->rA()] + cpu->core.regs[instr->rB()];
	if (cpu->write<float>(addr, cpu->core.fprs[instr->rS()].ps0)) {
		cpu->core.regs[instr->rA()] = addr;
	}
}

void PPCInstr_stfdux(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t addr = cpu->core.regs[instr->rA()] + cpu->core.regs[instr->rB()];
	if (cpu->write<double>(addr, cpu->core.fprs[instr->rS()].dbl)) {
		cpu->core.regs[instr->rA()] = addr;
	}
}

void PPCInstr_fmr(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].ps0 = cpu->core.fprs[instr->rB()].ps0;
}

void PPCInstr_frsp(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].ps0 = (float)cpu->core.fprs[instr->rB()].dbl;
}

void PPCInstr_fctiwz(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].iw1 = (int32_t)cpu->core.fprs[instr->rB()].ps0;
}

void PPCInstr_fneg(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].dbl = -cpu->core.fprs[instr->rB()].dbl;
}

void PPCInstr_fabs(PPCInstruction *instr, PPCProcessor *cpu) {
	float value = cpu->core.fprs[instr->rB()].ps0;
	if (value < 0) value = -value;
	cpu->core.fprs[instr->rD()].ps0 = value;
}

void PPCInstr_fmul(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].dbl = cpu->core.fprs[instr->rA()].dbl * cpu->core.fprs[instr->rC()].dbl;
}

void PPCInstr_fmuls(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].ps0 = cpu->core.fprs[instr->rA()].ps0 * cpu->core.fprs[instr->rC()].ps0;
}

void PPCInstr_fmadd(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].dbl = cpu->core.fprs[instr->rA()].dbl *
		cpu->core.fprs[instr->rC()].dbl + cpu->core.fprs[instr->rB()].dbl;
}

void PPCInstr_fmadds(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].ps0 = cpu->core.fprs[instr->rA()].ps0 *
		cpu->core.fprs[instr->rC()].ps0 + cpu->core.fprs[instr->rB()].ps0;
}

void PPCInstr_fmsub(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].dbl = cpu->core.fprs[instr->rA()].dbl * 
		cpu->core.fprs[instr->rC()].dbl - cpu->core.fprs[instr->rB()].dbl;
}

void PPCInstr_fmsubs(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].ps0 = cpu->core.fprs[instr->rA()].ps0 * 
		cpu->core.fprs[instr->rC()].ps0 - cpu->core.fprs[instr->rB()].ps0;
}

void PPCInstr_fnmadd(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].dbl = -(cpu->core.fprs[instr->rA()].dbl *
		cpu->core.fprs[instr->rC()].dbl + cpu->core.fprs[instr->rB()].dbl);
}

void PPCInstr_fnmadds(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].ps0 = -(cpu->core.fprs[instr->rA()].ps0 *
		cpu->core.fprs[instr->rC()].ps0 + cpu->core.fprs[instr->rB()].ps0);
}

void PPCInstr_fnmsub(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].dbl = -(cpu->core.fprs[instr->rA()].dbl * 
		cpu->core.fprs[instr->rC()].dbl - cpu->core.fprs[instr->rB()].dbl);
}

void PPCInstr_fnmsubs(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].ps0 = -(cpu->core.fprs[instr->rA()].ps0 * 
		cpu->core.fprs[instr->rC()].ps0 - cpu->core.fprs[instr->rB()].ps0);
}

void PPCInstr_fdiv(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].dbl = cpu->core.fprs[instr->rA()].dbl / cpu->core.fprs[instr->rB()].dbl;
}

void PPCInstr_fdivs(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].ps0 = cpu->core.fprs[instr->rA()].ps0 / cpu->core.fprs[instr->rB()].ps0;
}

void PPCInstr_fadd(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].dbl = cpu->core.fprs[instr->rA()].dbl + cpu->core.fprs[instr->rB()].dbl;
}

void PPCInstr_fadds(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].ps0 = cpu->core.fprs[instr->rA()].ps0 + cpu->core.fprs[instr->rB()].ps0;
}

void PPCInstr_fsub(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].dbl = cpu->core.fprs[instr->rA()].dbl - cpu->core.fprs[instr->rB()].dbl;
}

void PPCInstr_fsubs(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].ps0 = cpu->core.fprs[instr->rA()].ps0 - cpu->core.fprs[instr->rB()].ps0;
}

void PPCInstr_fsel(PPCInstruction *instr, PPCProcessor *cpu) {
	if (cpu->core.fprs[instr->rA()].dbl >= 0) {
		cpu->core.fprs[instr->rD()].dbl = cpu->core.fprs[instr->rC()].dbl;
	}
	else {
		cpu->core.fprs[instr->rD()].dbl = cpu->core.fprs[instr->rB()].dbl;
	}
}

void PPCInstr_frsqrte(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].ps0 = sqrt(cpu->core.fprs[instr->rB()].ps0);
}

void PPCInstr_fcmpu(PPCInstruction *instr, PPCProcessor *cpu) {
	float left = cpu->core.fprs[instr->rA()].ps0;
	float right = cpu->core.fprs[instr->rB()].ps0;
	cpu->core.cr.set(PPCCore::LT >> (4 * instr->crfD()), left < right);
	cpu->core.cr.set(PPCCore::GT >> (4 * instr->crfD()), left > right);
	cpu->core.cr.set(PPCCore::EQ >> (4 * instr->crfD()), left == right);
}

void PPCInstr_mffs(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].iw1 = cpu->core.fpscr;
}

void PPCInstr_mtfsf(PPCInstruction *instr, PPCProcessor *cpu) {
	uint8_t fm = (instr->value >> 17) & 0xFF;
	
	uint32_t mask = 0;
	for (int i = 0; i < 8; i++) {
		mask <<= 4;
		if (fm & 0x80) {
			mask |= 0xF;
		}
		fm <<= 1;
	}
	
	cpu->core.fpscr &= ~mask;
	cpu->core.fpscr |= cpu->core.fprs[instr->rD()].iw1 & mask;
}

/********** PAIRED SINGLE INSTRUCTIONS **********/

void psqLoad(PPCProcessor *cpu, uint32_t addr, int rD, int i, int w) {
	uint32_t config = cpu->core.sprs[PPCCore::GQR0 + i];
	int scale = (config >> 24) & 0x3F;
	int type = (config >> 16) & 7;
	if (type & 4) { //Dequantize
		runtime_error("Dequantization not supported");
	}
	
	if (w) {
		if (cpu->read<float>(addr, &cpu->core.fprs[i].ps0)) {
			cpu->core.fprs[i].ps1 = 1.0;
		}
	}
	else {
		if (cpu->read<float>(addr, &cpu->core.fprs[i].ps0)) {
			cpu->read<float>(addr+4, &cpu->core.fprs[i].ps1);
		}
	}
}

void psqStore(PPCProcessor *cpu, uint32_t addr, int rS, int i, int w) {
	uint32_t config = cpu->core.sprs[PPCCore::GQR0 + i];
	int scale = (config >> 8) & 0x3F;
	int type = config & 7;
	if (type & 4) { //Quantize
		runtime_error("Quantization not supported");
	}
	
	if (w) {
		cpu->write<float>(addr, cpu->core.fprs[i].ps0);
	}
	else {
		if (cpu->write<float>(addr, cpu->core.fprs[i].ps0)) {
			cpu->write<float>(addr+4, cpu->core.fprs[i].ps1);
		}
	}
}

void PPCInstr_psq_l(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t addr = (instr->rA() ? cpu->core.regs[instr->rA()] : 0) + instr->ps_d();
	psqLoad(cpu, addr, instr->rD(), instr->ps_i(), instr->ps_w());
}

void PPCInstr_psq_st(PPCInstruction *instr, PPCProcessor *cpu) {
	uint32_t addr = (instr->rA() ? cpu->core.regs[instr->rA()] : 0) + instr->ps_d();
	psqStore(cpu, addr, instr->rS(), instr->ps_i(), instr->ps_w());
}

void PPCInstr_ps_mr(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].ps0 = cpu->core.fprs[instr->rB()].ps0;
	cpu->core.fprs[instr->rD()].ps1 = cpu->core.fprs[instr->rB()].ps1;
}

void PPCInstr_ps_neg(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].ps0 = -cpu->core.fprs[instr->rB()].ps0;
	cpu->core.fprs[instr->rD()].ps1 = -cpu->core.fprs[instr->rB()].ps1;
}

void PPCInstr_ps_abs(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].ps0 = fabsf(cpu->core.fprs[instr->rB()].ps0);
	cpu->core.fprs[instr->rD()].ps1 = fabsf(cpu->core.fprs[instr->rB()].ps1);
}

void PPCInstr_ps_nabs(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].ps0 = -fabsf(cpu->core.fprs[instr->rB()].ps0);
	cpu->core.fprs[instr->rD()].ps1 = -fabsf(cpu->core.fprs[instr->rB()].ps1);
}

void PPCInstr_ps_merge00(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].ps0 = cpu->core.fprs[instr->rA()].ps0;
	cpu->core.fprs[instr->rD()].ps1 = cpu->core.fprs[instr->rB()].ps0;
}

void PPCInstr_ps_merge01(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].ps0 = cpu->core.fprs[instr->rA()].ps0;
	cpu->core.fprs[instr->rD()].ps1 = cpu->core.fprs[instr->rB()].ps1;
}

void PPCInstr_ps_merge10(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].ps0 = cpu->core.fprs[instr->rA()].ps1;
	cpu->core.fprs[instr->rD()].ps1 = cpu->core.fprs[instr->rB()].ps0;
}

void PPCInstr_ps_merge11(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].ps0 = cpu->core.fprs[instr->rA()].ps1;
	cpu->core.fprs[instr->rD()].ps1 = cpu->core.fprs[instr->rB()].ps1;
}

void PPCInstr_ps_add(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].ps0 = cpu->core.fprs[instr->rA()].ps0 + cpu->core.fprs[instr->rB()].ps0;
	cpu->core.fprs[instr->rD()].ps1 = cpu->core.fprs[instr->rA()].ps1 + cpu->core.fprs[instr->rB()].ps1;
}

void PPCInstr_ps_sub(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].ps0 = cpu->core.fprs[instr->rA()].ps0 - cpu->core.fprs[instr->rB()].ps0;
	cpu->core.fprs[instr->rD()].ps1 = cpu->core.fprs[instr->rA()].ps1 - cpu->core.fprs[instr->rB()].ps1;
}

void PPCInstr_ps_mul(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].ps0 = cpu->core.fprs[instr->rA()].ps0 * cpu->core.fprs[instr->rC()].ps0;
	cpu->core.fprs[instr->rD()].ps1 = cpu->core.fprs[instr->rA()].ps1 * cpu->core.fprs[instr->rC()].ps1;
}

void PPCInstr_ps_div(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].ps0 = cpu->core.fprs[instr->rA()].ps0 / cpu->core.fprs[instr->rB()].ps0;
	cpu->core.fprs[instr->rD()].ps1 = cpu->core.fprs[instr->rA()].ps1 / cpu->core.fprs[instr->rB()].ps1;
}

void PPCInstr_ps_madd(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].ps0 = cpu->core.fprs[instr->rA()].ps0 * cpu->core.fprs[instr->rC()].ps0 + cpu->core.fprs[instr->rB()].ps0;
	cpu->core.fprs[instr->rD()].ps1 = cpu->core.fprs[instr->rA()].ps1 * cpu->core.fprs[instr->rC()].ps1 + cpu->core.fprs[instr->rB()].ps1;
}

void PPCInstr_ps_msub(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].ps0 = cpu->core.fprs[instr->rA()].ps0 * cpu->core.fprs[instr->rC()].ps0 - cpu->core.fprs[instr->rB()].ps0;
	cpu->core.fprs[instr->rD()].ps1 = cpu->core.fprs[instr->rA()].ps1 * cpu->core.fprs[instr->rC()].ps1 - cpu->core.fprs[instr->rB()].ps1;
}

void PPCInstr_ps_nmadd(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].ps0 = -(cpu->core.fprs[instr->rA()].ps0 * cpu->core.fprs[instr->rC()].ps0 + cpu->core.fprs[instr->rB()].ps0);
	cpu->core.fprs[instr->rD()].ps1 = -(cpu->core.fprs[instr->rA()].ps1 * cpu->core.fprs[instr->rC()].ps1 + cpu->core.fprs[instr->rB()].ps1);
}

void PPCInstr_ps_nmsub(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].ps0 = -(cpu->core.fprs[instr->rA()].ps0 * cpu->core.fprs[instr->rC()].ps0 - cpu->core.fprs[instr->rB()].ps0);
	cpu->core.fprs[instr->rD()].ps1 = -(cpu->core.fprs[instr->rA()].ps1 * cpu->core.fprs[instr->rC()].ps1 - cpu->core.fprs[instr->rB()].ps1);
}

void PPCInstr_ps_sum0(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].ps0 = cpu->core.fprs[instr->rA()].ps0 + cpu->core.fprs[instr->rB()].ps1;
	cpu->core.fprs[instr->rD()].ps1 = cpu->core.fprs[instr->rC()].ps1;
}

void PPCInstr_ps_sum1(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].ps0 = cpu->core.fprs[instr->rC()].ps0;
	cpu->core.fprs[instr->rD()].ps1 = cpu->core.fprs[instr->rA()].ps0 + cpu->core.fprs[instr->rB()].ps1;
}

void PPCInstr_ps_muls0(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].ps0 = cpu->core.fprs[instr->rA()].ps0 * cpu->core.fprs[instr->rC()].ps0;
	cpu->core.fprs[instr->rD()].ps1 = cpu->core.fprs[instr->rA()].ps1 * cpu->core.fprs[instr->rC()].ps0;
}

void PPCInstr_ps_muls1(PPCInstruction *instr, PPCProcessor *cpu) {
	cpu->core.fprs[instr->rD()].ps0 = cpu->core.fprs[instr->rA()].ps0 * cpu->core.fprs[instr->rC()].ps1;
	cpu->core.fprs[instr->rD()].ps1 = cpu->core.fprs[instr->rA()].ps1 * cpu->core.fprs[instr->rC()].ps1;
}

void PPCInstruction::execute(PPCProcessor *cpu) {
	int type = opcode();
	if (type == 4) {
		int type = opcode2();
		if (type == 18) PPCInstr_ps_div(this, cpu);
		else if (type == 20) PPCInstr_ps_sub(this, cpu);
		else if (type == 21) PPCInstr_ps_add(this, cpu);
		else if (type == 40) PPCInstr_ps_neg(this, cpu);
		else if (type == 72) PPCInstr_ps_mr(this, cpu);
		else if (type == 136) PPCInstr_ps_nabs(this, cpu);
		else if (type == 264) PPCInstr_ps_abs(this, cpu);
		else if (type == 528) PPCInstr_ps_merge00(this, cpu);
		else if (type == 560) PPCInstr_ps_merge01(this, cpu);
		else if (type == 592) PPCInstr_ps_merge10(this, cpu);
		else if (type == 624) PPCInstr_ps_merge11(this, cpu);
		else {
			int type = opcode3();
			if (type == 10) PPCInstr_ps_sum0(this, cpu);
			else if (type == 11) PPCInstr_ps_sum1(this, cpu);
			else if (type == 12) PPCInstr_ps_muls0(this, cpu);
			else if (type == 13) PPCInstr_ps_muls1(this, cpu);
			else if (type == 25) PPCInstr_ps_mul(this, cpu);
			else if (type == 28) PPCInstr_ps_msub(this, cpu);
			else if (type == 29) PPCInstr_ps_madd(this, cpu);
			else if (type == 30) PPCInstr_ps_nmsub(this, cpu);
			else if (type == 31) PPCInstr_ps_nmadd(this, cpu);
		}
	}
	else if (type == 7) PPCInstr_mulli(this, cpu);
	else if (type == 8) PPCInstr_subfic(this, cpu);
	else if (type == 10) PPCInstr_cmpli(this, cpu);
	else if (type == 11) PPCInstr_cmpi(this, cpu);
	else if (type == 12) PPCInstr_addic(this, cpu);
	else if (type == 13) PPCInstr_addic_rc(this, cpu);
	else if (type == 17) PPCInstr_sc(this, cpu);
	else if (type == 19) {
		int type = opcode2();
		if (type == 16) PPCInstr_bclr(this, cpu);
		else if (type == 33) PPCInstr_crnor(this, cpu);
		else if (type == 50) PPCInstr_rfi(this, cpu);
		else if (type == 129) PPCInstr_crandc(this, cpu);
		else if (type == 193) PPCInstr_crxor(this, cpu);
		else if (type == 225) PPCInstr_crnand(this, cpu);
		else if (type == 257) PPCInstr_crand(this, cpu);
		else if (type == 289) PPCInstr_creqv(this, cpu);
		else if (type == 417) PPCInstr_crorc(this, cpu);
		else if (type == 449) PPCInstr_cror(this, cpu);
		else if (type == 528) PPCInstr_bcctr(this, cpu);
	}
	else if (type == 31) {
		int type = opcode2();
		if (type == 0) PPCInstr_cmp(this, cpu);
		else if (type == 8) PPCInstr_subfc(this, cpu);
		else if (type == 10) PPCInstr_addc(this, cpu);
		else if (type == 11) PPCInstr_mulhwu(this, cpu);
		else if (type == 19) PPCInstr_mfcr(this, cpu);
		else if (type == 20) PPCInstr_lwarx(this, cpu);
		else if (type == 23) PPCInstr_loadx<uint32_t>(this, cpu);
		else if (type == 24) PPCInstr_slw(this, cpu);
		else if (type == 26) PPCInstr_cntlzw(this, cpu);
		else if (type == 28) PPCInstr_and(this, cpu);
		else if (type == 32) PPCInstr_cmpl(this, cpu);
		else if (type == 40) PPCInstr_subf(this, cpu);
		else if (type == 55) PPCInstr_loadux<uint32_t>(this, cpu);
		else if (type == 60) PPCInstr_andc(this, cpu);
		else if (type == 75) PPCInstr_mulhw(this, cpu);
		else if (type == 83) PPCInstr_mfmsr(this, cpu);
		else if (type == 87) PPCInstr_loadx<uint8_t>(this, cpu);
		else if (type == 104) PPCInstr_neg(this, cpu);
		else if (type == 119) PPCInstr_loadux<uint8_t>(this, cpu);
		else if (type == 124) PPCInstr_nor(this, cpu);
		else if (type == 136) PPCInstr_subfe(this, cpu);
		else if (type == 138) PPCInstr_adde(this, cpu);
		else if (type == 144) PPCInstr_mtcrf(this, cpu);
		else if (type == 146) PPCInstr_mtmsr(this, cpu);
		else if (type == 150) PPCInstr_stwcx(this, cpu);
		else if (type == 151) PPCInstr_storex<uint32_t>(this, cpu);
		else if (type == 183) PPCInstr_storeux<uint32_t>(this, cpu);
		else if (type == 200) PPCInstr_subfze(this, cpu);
		else if (type == 202) PPCInstr_addze(this, cpu);
		else if (type == 210) PPCInstr_mtsr(this, cpu);
		else if (type == 215) PPCInstr_storex<uint8_t>(this, cpu);
		else if (type == 235) PPCInstr_mullw(this, cpu);
		else if (type == 247) PPCInstr_storeux<uint8_t>(this, cpu);
		else if (type == 279) PPCInstr_loadx<uint16_t>(this, cpu);
		else if (type == 306) PPCInstr_tlbie(this, cpu);
		else if (type == 311) PPCInstr_loadux<uint16_t>(this, cpu);
		else if (type == 316) PPCInstr_xor(this, cpu);
		else if (type == 339) PPCInstr_mfspr(this, cpu);
		else if (type == 343) PPCInstr_loadx<int16_t>(this, cpu);
		else if (type == 371) PPCInstr_mftb(this, cpu);
		else if (type == 375) PPCInstr_loadux<int16_t>(this, cpu);
		else if (type == 407) PPCInstr_storex<uint16_t>(this, cpu);
		else if (type == 439) PPCInstr_storeux<uint16_t>(this, cpu);
		else if (type == 444) PPCInstr_or(this, cpu);
		else if (type == 459) PPCInstr_divwu(this, cpu);
		else if (type == 467) PPCInstr_mtspr(this, cpu);
		else if (type == 491) PPCInstr_divw(this, cpu);
		else if (type == 536) PPCInstr_srw(this, cpu);
		else if (type == 567) PPCInstr_lfsux(this, cpu);
		else if (type == 595) PPCInstr_mfsr(this, cpu);
		else if (type == 631) PPCInstr_lfdux(this, cpu);
		else if (type == 695) PPCInstr_stfsux(this, cpu);
		else if (type == 759) PPCInstr_stfdux(this, cpu);
		else if (type == 792) PPCInstr_sraw(this, cpu);
		else if (type == 824) PPCInstr_srawi(this, cpu);
		else if (type == 922) PPCInstr_extsh(this, cpu);
		else if (type == 954) PPCInstr_extsb(this, cpu);
		else if (type == 982) PPCInstr_icbi(this, cpu);
		else if (type == 1014) PPCInstr_dcbz(this, cpu);
	}
	else if (type == 36) PPCInstr_store<uint32_t>(this, cpu);
	else if (type == 37) PPCInstr_storeu<uint32_t>(this, cpu);
	else if (type == 38) PPCInstr_store<uint8_t>(this, cpu);
	else if (type == 39) PPCInstr_storeu<uint8_t>(this, cpu);
	else if (type == 44) PPCInstr_store<uint16_t>(this, cpu);
	else if (type == 45) PPCInstr_storeu<uint16_t>(this, cpu);
	else if (type == 46) PPCInstr_lmw(this, cpu);
	else if (type == 47) PPCInstr_stmw(this, cpu);
	else if (type == 48) PPCInstr_lfs(this, cpu);
	else if (type == 49) PPCInstr_lfsu(this, cpu);
	else if (type == 50) PPCInstr_lfd(this, cpu);
	else if (type == 51) PPCInstr_lfdu(this, cpu);
	else if (type == 52) PPCInstr_stfs(this, cpu);
	else if (type == 53) PPCInstr_stfsu(this, cpu);
	else if (type == 54) PPCInstr_stfd(this, cpu);
	else if (type == 55) PPCInstr_stfdu(this, cpu);
	else if (type == 56) PPCInstr_psq_l(this, cpu);
	else if (type == 59) {
		int type = opcode3();
		if (type == 18) PPCInstr_fdivs(this, cpu);
		else if (type == 20) PPCInstr_fsubs(this, cpu);
		else if (type == 21) PPCInstr_fadds(this, cpu);
		else if (type == 25) PPCInstr_fmuls(this, cpu);
		else if (type == 28) PPCInstr_fmsubs(this, cpu);
		else if (type == 29) PPCInstr_fmadds(this, cpu);
		else if (type == 30) PPCInstr_fnmsubs(this, cpu);
		else if (type == 31) PPCInstr_fnmadds(this, cpu);
	}
	else if (type == 60) PPCInstr_psq_st(this, cpu);
	else if (type == 63) {
		int type = opcode2();
		if (type == 0) PPCInstr_fcmpu(this, cpu);
		else if (type == 12) PPCInstr_frsp(this, cpu);
		else if (type == 15) PPCInstr_fctiwz(this, cpu);
		else if (type == 18) PPCInstr_fdiv(this, cpu);
		else if (type == 20) PPCInstr_fsub(this, cpu);
		else if (type == 21) PPCInstr_fadd(this, cpu);
		else if (type == 40) PPCInstr_fneg(this, cpu);
		else if (type == 72) PPCInstr_fmr(this, cpu);
		else if (type == 264) PPCInstr_fabs(this, cpu);
		else if (type == 583) PPCInstr_mffs(this, cpu);
		else if (type == 711) PPCInstr_mtfsf(this, cpu);
		else {
			int type = opcode3();
			if (type == 23) PPCInstr_fsel(this, cpu);
			else if (type == 25) PPCInstr_fmul(this, cpu);
			else if (type == 26) PPCInstr_frsqrte(this, cpu);
			else if (type == 28) PPCInstr_fmsub(this, cpu);
			else if (type == 29) PPCInstr_fmadd(this, cpu);
			else if (type == 30) PPCInstr_fnmsub(this, cpu);
			else if (type == 31) PPCInstr_fnmadd(this, cpu);
		}
	}
}
