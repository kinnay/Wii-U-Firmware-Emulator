
#pragma once

#include <map>

#include <cstdint>


class MEMSeqController {
public:
	void reset();
	
	uint16_t read(uint16_t addr);
	void write(uint16_t addr, uint16_t value);
	
private:
	std::map<uint16_t, uint16_t> data;
};


class MEMConfigController {
public:
	void write(uint32_t addr, uint16_t value);
};


class MEMController {
public:
	enum Register {
		MEM_COMPAT = 0xD0B4200,
		MEM_COLSEL = 0xD0B4210,
		MEM_ROWSEL = 0xD0B4212,
		MEM_BANKSEL = 0xD0B4214,
		MEM_RANKSEL = 0xD0B4216,
		MEM_COLMSK = 0xD0B4218,
		MEM_ROWMSK = 0xD0B421A,
		MEM_BANKMSK = 0xD0B421C,
		MEM_RFSH = 0xD0B4226,
		MEM_FLUSH_MASK = 0xD0B4228,
		MEM_SEQRD_HWM = 0xD0B4268,
		MEM_SEQWR_HWM = 0xD0B426A,
		MEM_SEQCMD_HWM = 0xD0B426C,
		MEM_CPUAHM_WR_T = 0xD0B426E,
		MEM_DMAAHM_WR_T = 0xD0B4270,
		MEM_DMAAHM0_WR_T = 0xD0B4272,
		MEM_DMAAHM1_WR_T = 0xD0B4274,
		MEM_PI_WR_T = 0xD0B4276,
		MEM_PE_WR_T = 0xD0B4278,
		MEM_IO_WR_T = 0xD0B427A,
		MEM_DSP_WR_T = 0xD0B427C,
		MEM_ACC_WR_T = 0xD0B427E,
		MEM_ARB_MAXWR = 0xD0B4280,
		MEM_ARB_MINRD = 0xD0B4282,
		MEM_RDPR_PI = 0xD0B42A6,
		MEM_ARB_MISC = 0xD0B42B6,
		MEM_WRMUX = 0xD0B42BA,
		MEM_ARB_EXADDR = 0xD0B42C0,
		MEM_ARB_EXCMD = 0xD0B42C2,
		MEM_SEQ_DATA = 0xD0B42C4,
		MEM_SEQ_ADDR = 0xD0B42C6,
		MEM_EDRAM_REFRESH_CTRL = 0xD0B42CC,
		MEM_EDRAM_REFRESH_VAL = 0xD0B42CE,
		MEM_D8B42D0 = 0xD0B42D0,
		MEM_D8B42D2 = 0xD0B42D2,
		MEM_D8B42D6 = 0xD0B42D6,
		MEM_SEQ0_DATA = 0xD0B4300,
		MEM_SEQ0_ADDR = 0xD0B4302,
		MEM_SEQ0_CTRL = 0xD0B4306,
		MEM_BLOCK_MEM0_CFG = 0xD0B4400,
		MEM_BLOCK_MEM1_CFG = 0xD0B4402,
		MEM_BLOCK_MEM2_CFG = 0xD0B4404,
		
		MEM_PROT_START = 0xD0B440E,
		MEM_PROT_END = 0xD0B44C6,
		
		MEM_D8B44E0 = 0xD0B44E0,
		MEM_D8B44E2 = 0xD0B44E2,
		MEM_D8B44E4 = 0xD0B44E4,
		MEM_D8B44E6 = 0xD0B44E6,
		MEM_D8B44E8 = 0xD0B44E8,
		MEM_D8B44EA = 0xD0B44EA,
		
		MEM_CONFIG_START = 0xD0B4600,
		MEM_CONFIG_END = 0xD0B4648,
	};
	
	void reset();
	
	uint16_t read(uint32_t addr);
	void write(uint32_t addr, uint16_t value);
	
private:
	uint16_t compat;
	uint16_t seq_addr;
	uint16_t seq0_addr;
	uint16_t seq0_ctrl;
	uint16_t mem0_config;
	uint16_t mem1_config;
	uint16_t mem2_config;
	uint16_t d8b44e8;
	uint16_t d8b44ea;
	
	MEMSeqController seq;
	MEMSeqController seq0;
	
	MEMConfigController config[4];
};
