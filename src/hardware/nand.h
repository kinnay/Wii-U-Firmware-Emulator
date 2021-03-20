
#pragma once

#include <cstdint>


class PhysicalMemory;


class NANDBank {
public:
	enum Register {
		NAND_CTRL = 0,
		NAND_CONFIG = 4,
		NAND_ADDR1 = 8,
		NAND_ADDR2 = 0xC,
		NAND_DATABUF = 0x10,
		NAND_ECCBUF = 0x14
	};
	
	void prepare(PhysicalMemory *physmem, uint8_t *slc, uint8_t *slccmpt);
	void reset();
	
	void set_bank(bool cmpt);
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
	void process_config();
	
	bool check_interrupts();
	
private:
	void parse_addr(int flags);
	
	void process_ctrl(uint32_t value);
	void process_command(int command, int length);
	
	bool interrupt;
	
	uint32_t ctrl;
	uint32_t config;
	uint32_t addr1;
	uint32_t addr2;
	uint32_t databuf;
	uint32_t eccbuf;
	
	uint32_t pagenum;
	uint32_t pageoff;
	
	uint8_t *slc;
	uint8_t *slccmpt;
	
	uint8_t *file;
	
	PhysicalMemory *physmem;
};


class NANDController {
public:
	enum Register {
		NAND_BANK = 0xD010018,
		NAND_BANK_CTRL = 0xD010030,
		NAND_INT_MASK = 0xD010034,
		
		NAND_MAIN_START = 0xD010000,
		NAND_MAIN_END = 0xD010018,
		
		NAND_BANKS_START = 0xD010040,
		NAND_BANKS_END = 0xD010100
	};
	
	NANDController(PhysicalMemory *physmem);
	~NANDController();
	
	void reset();
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
	bool check_interrupts();
	
private:
	uint32_t bank_ctrl;
	
	NANDBank main;
	NANDBank banks[8];
	
	uint8_t *slc;
	uint8_t *slccmpt;
};
