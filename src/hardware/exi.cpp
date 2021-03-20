
#include "hardware/exi.h"

#include "common/logger.h"
#include "common/endian.h"


void RTCController::reset() {
	data = 0;
	offset = 0;
	words = 0;
	
	counter = 0;
	offtimer = 0;
	control0 = 0;
	control1 = 0;
	
	sramidx = 0;
}

uint32_t RTCController::read() {
	return data;
}

uint32_t RTCController::read_value(uint32_t offset) {
	switch (offset) {
		case 0x20000000: return counter;
		case 0x21000100: return offtimer;
		case 0x21000C00: return control0;
		case 0x21000D00: return control1;
	}
	
	Logger::warning("Unknown rtc read: 0x%08X", offset);
	return 0;
}

void RTCController::process_write(uint32_t offset, uint32_t value) {
	if (offset == 0x20000100) {
		sram[sramidx] = Endian::swap32(value);
		sramidx = (sramidx + 1) % 16;
	}
	else if (offset == 0x21000100) offtimer = value & 0x3FFFFFFF;
	else if (offset == 0x21000C00) control0 = value;
	else if (offset == 0x21000D00) control1 = value;
	else {
		Logger::warning("Unknown rtc write: 0x%08X (0x%08X)", offset, value);
	}
}

void RTCController::write(uint32_t value) {
	if (words > 0) {
		process_write(offset, value);
		words--;
	}
	else {
		if (value & 0x80000000) {
			offset = value & ~0x80000000;			
			if (offset == 0x20000100) {
				words = 16;
			}
			else {
				words = 1;
			}
		}
		else {
			data = read_value(value);
		}
	}
}


void EXIController::reset() {
	csr = 0;
	data = 0;
	
	interrupt = false;
	
	rtc.reset();
}

bool EXIController::check_interrupts() {
	bool ints = interrupt;
	interrupt = false;
	return ints;
}

uint32_t EXIController::read(uint32_t addr) {
	switch (addr) {
		case EXI_CSR: return csr;
		case EXI_DATA: return data;
	}
	
	Logger::warning("Unknown exi memory read: 0x%X", addr);
	return 0;
}

void EXIController::write(uint32_t addr, uint32_t value) {
	if (addr == EXI_CSR) csr = value;
	else if (addr == EXI_CR) {
		if (value == 0x31) {
			data = rtc.read();
			interrupt = true;
		}
		else if (value == 0x35) {
			csr |= 4;
			rtc.write(data);
			interrupt = true;
		}
		else {
			Logger::warning("Unknown exi command: 0x%08X", value);
		}
	}
	else if (addr == EXI_DATA) data = value;
	else {
		Logger::warning("Unknown exi memory write: 0x%X (0x%08X)", addr, value);
	}
}