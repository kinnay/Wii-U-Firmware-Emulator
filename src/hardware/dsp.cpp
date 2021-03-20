
#include "hardware/dsp.h"
#include "common/logger.h"


void DSPController::reset() {
	mailbox_in = 0;
	mailbox_out = 0x80000000;
	
	halted = true;
	int_status = false;
	int_mask = false;
	
	state = STATE_NEXT;
}

uint32_t DSPController::read(uint32_t addr) {
	switch (addr) {
		case DSP_MAILBOX_IN_H: return 0;
		case DSP_MAILBOX_OUT_H: return mailbox_out >> 16;
		case DSP_MAILBOX_OUT_L: return mailbox_out & 0xFFFF;
		case DSP_CONTROL_STATUS:
			return (halted << 2) | (int_status << 7) | (int_mask << 8);
	}
	Logger::warning("Unknown dsp read: 0x%X", addr);
	return 0;
}

void DSPController::write(uint32_t addr, uint32_t value) {
	if (addr == DSP_MAILBOX_IN_H) mailbox_in = (mailbox_in & 0xFFFF) | (value << 16);
	else if (addr == DSP_MAILBOX_IN_L) {
		mailbox_in = (mailbox_in & ~0xFFFF) | value;
		accept(mailbox_in);
	}
	else if (addr == DSP_CONTROL_STATUS) {
		halted = value & 4;
		int_mask = value & 0x100;
		
		if (value & 0x80) {
			int_status = false;
		}
		if (value & 0x801) reset();
	}
	else {
		Logger::warning("Unknown dsp write: 0x%X (0x%08X)", addr, value);
	}
}

bool DSPController::check_interrupts() {
	return int_status && int_mask;
}

void DSPController::send_mail(MessageOut message) {
	mailbox_out = message;
	int_status = true;
}

void DSPController::accept(uint32_t mail) {
	if (state == STATE_NEXT) {
		message = mail;
		state = STATE_ARGUMENT;
	}
	else {
		process(message, mail);
		state = STATE_NEXT;
	}
}

void DSPController::process(uint32_t message, uint32_t arg) {
	if (message == 0x80F3D001) {
		if (arg & 0x10) {
			send_mail(DSP_INIT);
		}
	}
	Logger::warning("Unknown dsp message: 0x%08X 0x%08X", message, arg);
}
