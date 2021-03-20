
#pragma once

#include <cstdint>


class AHCIController {
public:
	enum Register {
		AHCI_HBA_CONTROL = 0xD160404,
		AHCI_HBA_INT_STATUS = 0xD160408,
		
		AHCI_CMD_BASE = 0xD160500,
		AHCI_CMD_BASE_HI = 0xD160504,
		AHCI_FIS_BASE = 0xD160508,
		AHCI_FIS_BASE_HI = 0xD16050C,
		AHCI_INT_STATUS = 0xD160510,
		AHCI_INT_ENABLE = 0xD160514,
		AHCI_CMD_STATUS = 0xD160518,
		AHCI_SATA_STATUS = 0xD160528,
		AHCI_SATA_CONTROL = 0xD16052C,
		
		AHCI_SATA_INT_STATE = 0xD160800,
		AHCI_SATA_INT_MASK = 0xD160804,
		
		AHCI_D160884 = 0xD160884,
		AHCI_D160888 = 0xD160888,
		AHCI_D160894 = 0xD160894,
		AHCI_D160898 = 0xD160898
	};
	
	void reset();
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
private:
	uint32_t cmd_status;
	uint32_t sata_control;
	
	uint32_t sata_int_mask;
	uint32_t sata_int_state;
	
	uint32_t d160888;
	uint32_t d160894;
	uint32_t d160898;
};
