
#pragma once

#include <cstdint>


class Hardware;

class IRQController {
public:
	enum Register {
		IRQ_FLAG_ALL = 0,
		IRQ_FLAG_LT = 4,
		IRQ_MASK_ALL = 8,
		IRQ_MASK_LT = 0xC,
		FIQ_MASK_ALL = 0x10,
		FIQ_MASK_LT = 0x14
	};
	
	IRQController(Hardware *hardware, int index);
	
	void reset();
	void update();
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
	void trigger_irq_all(int type);
	void trigger_irq_lt(int type);
	
	bool check_interrupts();
	
private:
	Hardware *hardware;
	int index;
	
	uint32_t intsr_all;
	uint32_t intsr_lt;
	uint32_t intmr_all;
	uint32_t intmr_lt;
	uint32_t fiq_all;
	uint32_t fiq_lt;
};
