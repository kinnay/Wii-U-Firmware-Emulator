
#include "hardware/irq.h"
#include "hardware.h"
#include "common/logger.h"


IRQController::IRQController(Hardware *hardware, int index) {
	this->hardware = hardware;
	this->index = index;
}

void IRQController::reset() {
	intsr_all = 0;
	intsr_lt = 0;
	intmr_all = 0;
	intmr_lt = 0;
	fiq_all = 0;
	fiq_lt = 0;
}

uint32_t IRQController::read(uint32_t addr) {
	switch (addr) {
		case IRQ_FLAG_ALL: return intsr_all;
		case IRQ_FLAG_LT: return intsr_lt;
		case IRQ_MASK_ALL: return intmr_all;
		case IRQ_MASK_LT: return intmr_lt;
		case FIQ_MASK_ALL: return fiq_all;
		case FIQ_MASK_LT: return fiq_lt;
	}
	
	Logger::warning("Unknown irq read: 0x%X", addr);
	return 0;
}

void IRQController::write(uint32_t addr, uint32_t value) {
	if (addr == IRQ_FLAG_ALL) intsr_all &= ~value;
	else if (addr == IRQ_FLAG_LT) intsr_lt &= ~value;
	else if (addr == IRQ_MASK_ALL) {
		intmr_all = value;
		check_interrupts();
	}
	else if (addr == IRQ_MASK_LT) {
		intmr_lt = value;
		check_interrupts();
	}
	else if (addr == FIQ_MASK_ALL) fiq_all = value;
	else if (addr == FIQ_MASK_LT) fiq_lt = value;
	else {
		Logger::warning("Unknown irq write: 0x%X (0x%08X)", addr, value);
	}
}

void IRQController::trigger_irq_all(int type) {
	intsr_all |= 1 << type;
}

void IRQController::trigger_irq_lt(int type) {
	intsr_lt |= 1 << type;
}

bool IRQController::check_interrupts() {
	return (intsr_all & intmr_all) || (intsr_lt & intmr_lt);
}

void IRQController::update() {
	if (check_interrupts()) {
		if (index != -1) {
			hardware->trigger_irq_pi(Hardware::PPC0 + index, 24);
		}
	}
}
