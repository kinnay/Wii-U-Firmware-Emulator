
#include "hardware/mem.h"

#include "common/logger.h"


void MEMSeqController::reset() {
	data.clear();
}

uint16_t MEMSeqController::read(uint16_t addr) {
	return data[addr];
}

void MEMSeqController::write(uint16_t addr, uint16_t value) {
	data[addr] = value;
}


void MEMConfigController::write(uint32_t addr, uint16_t value) {}


void MEMController::reset() {
	compat = 0;
	seq_addr = 0;
	seq0_addr = 0;
	seq0_ctrl = 0;
	mem0_config = 0;
	mem1_config = 0;
	mem2_config = 0;
	d8b44e8 = 0;
	d8b44ea = 0;
	
	seq.reset();
	seq0.reset();
}

uint16_t MEMController::read(uint32_t addr) {
	switch (addr) {
		case MEM_COMPAT: return compat;
		case MEM_FLUSH_MASK: return 0;
		case MEM_SEQ_DATA: return seq.read(seq_addr);
		case MEM_SEQ0_DATA: return seq0.read(seq0_addr);
		case MEM_SEQ0_CTRL: return seq0_ctrl;
		case MEM_BLOCK_MEM0_CFG: return mem0_config;
		case MEM_BLOCK_MEM1_CFG: return mem1_config;
		case MEM_BLOCK_MEM2_CFG: return mem2_config;
		case MEM_D8B44E8: return d8b44e8;
		case MEM_D8B44EA: return d8b44ea;
	}
	
	Logger::warning("Unknown mem read: 0x%X", addr);
	return 0;
}

void MEMController::write(uint32_t addr, uint16_t value) {
	if (addr == MEM_COMPAT) compat = value;
	else if (addr == MEM_COLSEL) {}
	else if (addr == MEM_ROWSEL) {}
	else if (addr == MEM_BANKSEL) {}
	else if (addr == MEM_RANKSEL) {}
	else if (addr == MEM_COLMSK) {}
	else if (addr == MEM_ROWMSK) {}
	else if (addr == MEM_BANKMSK) {}
	else if (addr == MEM_RFSH) {}
	else if (addr == MEM_FLUSH_MASK) {}
	else if (addr == MEM_SEQRD_HWM) {}
	else if (addr == MEM_SEQWR_HWM) {}
	else if (addr == MEM_SEQCMD_HWM) {}
	else if (addr == MEM_CPUAHM_WR_T) {}
	else if (addr == MEM_DMAAHM_WR_T) {}
	else if (addr == MEM_DMAAHM0_WR_T) {}
	else if (addr == MEM_DMAAHM1_WR_T) {}
	else if (addr == MEM_PI_WR_T) {}
	else if (addr == MEM_PE_WR_T) {}
	else if (addr == MEM_IO_WR_T) {}
	else if (addr == MEM_DSP_WR_T) {}
	else if (addr == MEM_ACC_WR_T) {}
	else if (addr == MEM_ARB_MAXWR) {}
	else if (addr == MEM_ARB_MINRD) {}
	else if (addr == MEM_RDPR_PI) {}
	else if (addr == MEM_ARB_MISC) {}
	else if (addr == MEM_WRMUX) {}
	else if (addr == MEM_ARB_EXADDR) {}
	else if (addr == MEM_ARB_EXCMD) {}
	else if (addr == MEM_SEQ_DATA) seq.write(seq_addr, value);
	else if (addr == MEM_SEQ_ADDR) seq_addr = value;
	else if (addr == MEM_EDRAM_REFRESH_CTRL) {}
	else if (addr == MEM_EDRAM_REFRESH_VAL) {}
	else if (addr == MEM_D8B42D0) {}
	else if (addr == MEM_D8B42D2) {}
	else if (addr == MEM_D8B42D6) {}
	else if (addr == MEM_SEQ0_DATA) seq0.write(seq0_addr, value);
	else if (addr == MEM_SEQ0_ADDR) seq0_addr = value;
	else if (addr == MEM_SEQ0_CTRL) seq0_ctrl = value;
	else if (addr == MEM_BLOCK_MEM0_CFG) mem0_config = value;
	else if (addr == MEM_BLOCK_MEM1_CFG) mem1_config = value;
	else if (addr == MEM_BLOCK_MEM2_CFG) mem2_config = value;
	else if (MEM_PROT_START <= addr && addr < MEM_PROT_END) {}
	else if (addr == MEM_D8B44E0) {}
	else if (addr == MEM_D8B44E2) {}
	else if (addr == MEM_D8B44E4) {}
	else if (addr == MEM_D8B44E6) {}
	else if (addr == MEM_D8B44E8) d8b44e8 = value;
	else if (addr == MEM_D8B44EA) d8b44ea = value;
	else if (MEM_CONFIG_START <= addr && addr < MEM_CONFIG_END) {
		addr -= MEM_CONFIG_START;
		config[addr / 0x12].write(addr % 0x12, value);
	}
	else {
		Logger::warning("Unknown mem write: 0x%X (0x%08X)", addr, value);
	}
}
