
#include "hardware/sha.h"

#include "common/logger.h"

#include "physicalmemory.h"


SHAController::SHAController(PhysicalMemory *physmem) {
	this->physmem = physmem;
}

void SHAController::reset() {
	interrupt = false;
	
	ctrl = 0;
	src = 0;
}

bool SHAController::check_interrupts() {
	bool ints = interrupt;
	interrupt = false;
	return ints;
}

uint32_t SHAController::read(uint32_t addr) {
	switch (addr) {
		case SHA_CTRL: return ctrl;
		case SHA_H0: return sha1.h0;
		case SHA_H1: return sha1.h1;
		case SHA_H2: return sha1.h2;
		case SHA_H3: return sha1.h3;
		case SHA_H4: return sha1.h4;
	}
	
	Logger::warning("Unknown sha read: 0x%X", addr);
	return 0;
}

void SHAController::write(uint32_t addr, uint32_t value) {
	if (addr == SHA_CTRL) {
		ctrl = value & ~0x80000000;
		if (value & 0x80000000) {
			int blocks = (value & 0x3FF) + 1;
			for (int i = 0; i < blocks; i++) {
				char block[0x40];
				physmem->read(src, block, 0x40);
				sha1.update(block);
				src += 0x40;
			}
			
			if (value & 0x40000000) {
				interrupt = true;
			}
		}
	}
	else if (addr == SHA_SRC) src = value;
	else if (addr == SHA_H0) sha1.h0 = value;
	else if (addr == SHA_H1) sha1.h1 = value;
	else if (addr == SHA_H2) sha1.h2 = value;
	else if (addr == SHA_H3) sha1.h3 = value;
	else if (addr == SHA_H4) sha1.h4 = value;
	else {
		Logger::warning("Unknown sha write: 0x%X (0x%08X)", addr, value);
	}
}