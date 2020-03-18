
#include "hardware/pi.h"
#include "hardware.h"
#include "physicalmemory.h"


WGController::WGController(PhysicalMemory *physmem) {
	this->physmem = physmem;
}

void WGController::reset() {
	base = 0;
	top = 0;
	ptr = 0;
	threshold = 0;
	index = 0;
}

uint32_t WGController::read(uint32_t addr) {
	switch (addr) {
		case WG_BASE: return base;
		case WG_TOP: return top;
		case WG_PTR: return ptr;
		case WG_THRESHOLD: return threshold;
	}
	Logger::warning("Unknown wg read: 0x%X", addr);
	return 0;
}

void WGController::write(uint32_t addr, uint32_t value) {
	if (addr == WG_BASE) base = value;
	else if (addr == WG_TOP) top = value;
	else if (addr == WG_PTR) ptr = value;
	else if (addr == WG_THRESHOLD) threshold = value;
	else {
		Logger::warning("Unknown wg write: 0x%X (0x%08X)", addr, value);
	}
}

void WGController::write_data(uint32_t value) {
	buffer[index++] = value;
	if (index == 8) {
		for (int i = 0; i < 8; i++) {
			physmem->write<uint32_t>(ptr, buffer[i]);
			ptr += 4;
		}
		index = 0;
		if (ptr >= threshold) {
			ptr = base;
		}
	}
}


PIInterruptController::PIInterruptController(Hardware *hardware) {
	this->hardware = hardware;
}

void PIInterruptController::reset() {
	intsr = 0;
	intmr = 0;
}

void PIInterruptController::update() {
	intsr = 0;
}

uint32_t PIInterruptController::read(uint32_t addr) {
	switch (addr) {
		case PI_INTSR: return intsr;
		case PI_INTMR: return intmr;
	}
	
	Logger::warning("Unknown pi interrupt read: 0x%X", addr);
	return 0;
}

void PIInterruptController::write(uint32_t addr, uint32_t value) {
	if (addr == PI_INTSR) intsr &= ~value;
	else if (addr == PI_INTMR) intmr = value;
	else {
		Logger::warning("Unknown pi interrupt write: 0x%X (0x%08X)", addr, value);
	}
}

void PIInterruptController::trigger_irq(int irq) {
	intsr |= 1 << irq;
}

bool PIInterruptController::check_interrupts() {
	return intsr & intmr;
}


PIController::PIController(Hardware *hardware, PhysicalMemory *physmem) :
	wg {physmem, physmem, physmem},
	interrupt {hardware, hardware, hardware}
	{}

void PIController::reset() {
	for (int i = 0; i < 3; i++) wg[i].reset();
	for (int i = 0; i < 3; i++) interrupt[i].reset();
}

void PIController::update() {
	for (int i = 0; i < 3; i++) {
		interrupt[i].update();
	}
}

uint32_t PIController::read(uint32_t addr) {
	if (PI_WG_START <= addr && addr < PI_WG_END) {
		addr -= PI_WG_START;
		return wg[addr / 0x10].read(addr % 0x10);
	}
	if (PI_INTERRUPT_START <= addr && addr < PI_INTERRUPT_END) {
		addr -= PI_INTERRUPT_START;
		return interrupt[addr / 8].read(addr % 8);
	}
	Logger::warning("Unknown pi read: 0x%X", addr);
	return 0;
}

void PIController::write(uint32_t addr, uint32_t value) {
	if (PI_WG_START <= addr && addr < PI_WG_END) {
		addr -= PI_WG_START;
		wg[addr / 0x10].write(addr % 0x10, value);
	}
	else if (PI_INTERRUPT_START <= addr && addr < PI_INTERRUPT_END) {
		addr -= PI_INTERRUPT_START;
		interrupt[addr / 8].write(addr % 8, value);
	}
	else {
		Logger::warning("Unknown pi write: 0x%X (0x%08X)", addr, value);
	}
}

void PIController::trigger_irq(int core, int irq) {
	interrupt[core].trigger_irq(irq);
}
bool PIController::check_interrupts(int core) {
	return interrupt[core].check_interrupts();
}
