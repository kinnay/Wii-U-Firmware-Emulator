
#pragma once

#include "common/sha1.h"


class PhysicalMemory;

class SHAController {
public:
	enum Register {
		SHA_CTRL = 0,
		SHA_SRC = 4,
		SHA_H0 = 8,
		SHA_H1 = 0xC,
		SHA_H2 = 0x10,
		SHA_H3 = 0x14,
		SHA_H4 = 0x18
	};
	
	SHAController(PhysicalMemory *physmem);
	
	void reset();
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
	bool check_interrupts();
	
private:
	bool interrupt;
	
	uint32_t ctrl;
	uint32_t src;

	PhysicalMemory *physmem;
	
	SHA1 sha1;
};
