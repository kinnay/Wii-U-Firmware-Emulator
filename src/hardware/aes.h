
#pragma once

#include <openssl/aes.h>

#include <cstdint>


class PhysicalMemory;


class AESController {
public:
	enum Register {
		AES_CTRL = 0,
		AES_SRC = 4,
		AES_DEST = 8,
		AES_KEY = 0xC,
		AES_IV = 0x10
	};
	
	AESController(PhysicalMemory *physmem);
	
	void reset();
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
	bool check_interrupts();
	
private:
	bool interrupt;
	
	uint32_t ctrl;
	uint32_t src;
	uint32_t dest;
	uint32_t key[4];
	uint32_t iv[4];
	
	::AES_KEY aeskey;
	
	PhysicalMemory *physmem;
};
