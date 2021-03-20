
#pragma once

#include <cstdint>

class AHMNController {
public:
	enum Register {
		AHMN_REVISION = 0xD0B0010,
		AHMN_MEM0_CONFIG = 0xD0B0800,
		AHMN_MEM1_CONFIG = 0xD0B0804,
		AHMN_MEM2_CONFIG = 0xD0B0808,
		AHMN_RDBI_MASK = 0xD0B080C,
		AHMN_INTMSK = 0xD0B0820,
		AHMN_INTSTS = 0xD0B0824,
		AHMN_D8B0840 = 0xD0B0840,
		AHMN_D8B0844 = 0xD0B0844,
		AHMN_TRANSFER_STATE = 0xD0B0850,
		AHMN_WORKAROUND = 0xD0B0854,
		
		AHMN_MEM0_START = 0xD0B0900,
		AHMN_MEM0_END = 0xD0B0980,
		
		AHMN_MEM1_START = 0xD0B0A00,
		AHMN_MEM1_END = 0xD0B0C00,
		
		AHMN_MEM2_START = 0xD0B0C00,
		AHMN_MEM2_END = 0xD0B1000
	};
	
	void reset();
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
private:
	uint32_t mem0_config;
	uint32_t mem1_config;
	uint32_t mem2_config;
	uint32_t workaround;
	
	uint32_t mem0[0x20];
	uint32_t mem1[0x80];
	uint32_t mem2[0x100];
};
