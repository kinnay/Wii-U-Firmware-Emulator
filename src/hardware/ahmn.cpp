
#include "hardware/ahmn.h"

#include "common/logger.h"

void AHMNController::reset() {
	mem0_config = 0;
	mem1_config = 0;
	mem2_config = 0;
	workaround = 0;
}

uint32_t AHMNController::read(uint32_t addr) {
	switch (addr) {
		case AHMN_MEM0_CONFIG: return mem0_config;
		case AHMN_MEM1_CONFIG: return mem1_config;
		case AHMN_MEM2_CONFIG: return mem2_config;
		case AHMN_TRANSFER_STATE: return 0;
		case AHMN_WORKAROUND: return workaround;
	}
	
	if (AHMN_MEM0_START <= addr && addr < AHMN_MEM0_END) return mem0[(addr - AHMN_MEM0_START) / 4];
	if (AHMN_MEM1_START <= addr && addr < AHMN_MEM1_END) return mem1[(addr - AHMN_MEM1_START) / 4];
	if (AHMN_MEM2_START <= addr && addr < AHMN_MEM2_END) return mem2[(addr - AHMN_MEM2_START) / 4];
	
	Logger::warning("Unknown ahmn read: 0x%X", addr);
	return 0;
}

void AHMNController::write(uint32_t addr, uint32_t value) {
	if (addr == AHMN_REVISION) {}
	else if (addr == AHMN_MEM0_CONFIG) mem0_config = value;
	else if (addr == AHMN_MEM1_CONFIG) mem1_config = value;
	else if (addr == AHMN_MEM2_CONFIG) mem2_config = value;
	else if (addr == AHMN_RDBI_MASK) {}
	else if (addr == AHMN_INTMSK) {}
	else if (addr == AHMN_INTSTS) {}
	else if (addr == AHMN_D8B0840) {}
	else if (addr == AHMN_D8B0844) {}
	else if (addr == AHMN_WORKAROUND) workaround = value;
	else if (AHMN_MEM0_START <= addr && addr < AHMN_MEM0_END) mem0[(addr - AHMN_MEM0_START) / 4] = value ^ 0x80000000;
	else if (AHMN_MEM1_START <= addr && addr < AHMN_MEM1_END) mem1[(addr - AHMN_MEM1_START) / 4] = value ^ 0x80000000;
	else if (AHMN_MEM2_START <= addr && addr < AHMN_MEM2_END) mem2[(addr - AHMN_MEM2_START) / 4] = value ^ 0x80000000;
	else {
		Logger::warning("Unknown ahmn write: 0x%X (0x%08X)", addr, value);
	}
}
