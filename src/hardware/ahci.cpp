
#include "hardware/ahci.h"
#include "common/logger.h"

void AHCIController::reset() {
	cmd_status = 0;
	sata_control = 0;
}

uint32_t AHCIController::read(uint32_t addr) {
	switch (addr) {
		case AHCI_SATA_STATUS: return 3;
	}
	
	Logger::warning("Unknown ahci read: 0x%X", addr);
	return 0;
}

void AHCIController::write(uint32_t addr, uint32_t value) {
	if (addr == AHCI_CMD_STATUS) {
		cmd_status = value;
		if (value & 1) {
			Logger::warning("AHCI command processing not implemented");
		}
	}
	else if (addr == AHCI_SATA_CONTROL) sata_control = value;
	else {
		Logger::warning("Unknown ahci write: 0x%X (0x%08X)", addr, value);
	}
}
