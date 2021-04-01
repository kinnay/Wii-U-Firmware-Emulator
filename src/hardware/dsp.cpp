
#include "hardware/dsp.h"

#include "common/logger.h"

#include "emulator.h"


DSPController::DSPController(Emulator *emulator) {
	interpreter = &emulator->dsp;
}

void DSPController::reset() {
	int_enabled = false;
	
	interpreter->reset();
}

void DSPController::update() {
	if (interpreter->isEnabled() && !interpreter->isPaused()) {
		interpreter->step();
	}
}

uint16_t DSPController::read(uint32_t addr) {
	switch (addr) {
		case DSP_MAILBOX_IN_H: return interpreter->mailbox_in_h;
		case DSP_MAILBOX_OUT_H: return interpreter->mailbox_out_h;
		case DSP_MAILBOX_OUT_L:
			interpreter->mailbox_out_h &= 0x7FFF;
			return interpreter->mailbox_out_l;
		case DSP_CONTROL_STATUS:
			bool halted = !interpreter->isEnabled();
			return (halted << 2) | (interpreter->irq << 7) | (int_enabled << 8);
	}
	Logger::warning("Unknown dsp read: 0x%X", addr);
	return 0;
}

void DSPController::write(uint32_t addr, uint16_t value) {
	if (addr == DSP_MAILBOX_IN_H) interpreter->mailbox_in_h = value;
	else if (addr == DSP_MAILBOX_IN_L) {
		interpreter->mailbox_in_l = value;
		interpreter->mailbox_in_h |= 0x8000;
	}
	else if (addr == DSP_CONTROL_STATUS) {
		interpreter->setEnabled(!(value & 4));
		
		int_enabled = value & 0x100;
		if (value & 0x80) {
			interpreter->irq = false;
		}
		
		if (value & 1) {
			interpreter->reset();
		}
	}
	else {
		Logger::warning("Unknown dsp write: 0x%X (0x%04X)", addr, value);
	}
}

bool DSPController::check_interrupts() {
	return interpreter->irq && int_enabled;
}
