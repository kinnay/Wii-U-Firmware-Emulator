
#pragma once

#include <cstdint>


class PhysicalMemory;


class WGController {
public:
	enum Register {
		WG_BASE = 0,
		WG_TOP = 4,
		WG_PTR = 8,
		WG_THRESHOLD = 0xC
	};
	
	WGController(PhysicalMemory *physmem);
	
	void reset();
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
	void write_data(uint32_t data);
	
private:
	PhysicalMemory *physmem;
	
	int index;
	uint32_t base;
	uint32_t top;
	uint32_t ptr;
	uint32_t threshold;
	
	uint32_t buffer[8];
};


class PIInterruptController {
public:
	enum Register {
		PI_INTSR = 0,
		PI_INTMR = 4
	};
	
	void reset();
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
	void set_irq(int irq, bool state);
	
	bool check_interrupts();
	
private:
	uint32_t intsr;
	uint32_t intmr;
};


class PIController {
public:
	enum Register {
		PI_WG_START = 0xC000040,
		PI_WG_END = 0xC000070,
		
		PI_INTERRUPT_START = 0xC000078,
		PI_INTERRUPT_END = 0xC000090
	};
	
	PIController(PhysicalMemory *physmem);
	
	void reset();
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
	void set_irq(int irq, bool state);
	void set_irq(int core, int irq, bool state);
	
	bool check_interrupts(int core);
	
	WGController wg[3];
	
private:
	PIInterruptController interrupt[3];
};
