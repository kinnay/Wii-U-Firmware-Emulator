
#include "hardware/ahci.h"
#include "common/logger.h"

void AHCIController::reset() {
	cmd_status = 0;
	sata_control = 0;
	
	sata_int_state = 0;
	sata_int_mask = 0;
	
	d160888 = 0;
	d160894 = 0;
	d160898 = 0;
}

uint32_t AHCIController::read(uint32_t addr) {
	switch (addr) {
		case AHCI_SATA_STATUS: return 3;
		
		case AHCI_SATA_INT_STATE: return sata_int_state;
		case AHCI_SATA_INT_MASK: return sata_int_mask;

		case AHCI_D160888: return d160888;
		case AHCI_D160894: return d160894;
		case AHCI_D160898: return d160898;
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
	
	else if (addr == AHCI_SATA_INT_STATE) sata_int_state &= ~value;
	else if (addr == AHCI_SATA_INT_MASK) sata_int_mask = value;
	
	else if (addr == AHCI_D160884) {}
	else if (addr == AHCI_D160888) d160888 = value;
	else if (addr == AHCI_D160894) d160894 = value;
	else if (addr == AHCI_D160898) d160898 = value;
	
	else {
		Logger::warning("Unknown ahci write: 0x%X (0x%08X)", addr, value);
	}
}
