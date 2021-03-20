
#pragma once

#include <cstdint>


class RTCController {
public:
	void reset();
	
	uint32_t read();
	void write(uint32_t value);
	
private:
	uint32_t read_value(uint32_t offset);
	void process_write(uint32_t offset, uint32_t value);
	
	int words;
	uint32_t offset;
	uint32_t data;
	
	uint32_t counter;
	uint32_t offtimer;
	uint32_t control0;
	uint32_t control1;
	
	uint32_t sram[16];
	int sramidx;
};


class EXIController {
public:
	enum Register {
		EXI_CSR = 0,
		EXI_CR = 0xC,
		EXI_DATA = 0x10
	};
	
	void reset();
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
	bool check_interrupts();
	
private:
	bool interrupt;
	
	RTCController rtc;
	
	uint32_t csr;
	uint32_t data;
};
