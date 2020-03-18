
#pragma once

#include <cstdint>


class AHCIController {
public:
	enum Register {
		AHCI_CMD_STATUS = 0xD160518,
		AHCI_SATA_STATUS = 0xD160528,
		AHCI_SATA_CONTROL = 0xD16052C
	};
	
	void reset();
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
private:
	uint32_t cmd_status;
	uint32_t sata_control;
};
