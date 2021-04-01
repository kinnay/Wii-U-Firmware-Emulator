
#pragma once

#include "cpu/dsp.h"

#include <cstdint>


class Emulator;


class DSPController {
public:
	enum Register {
		DSP_MAILBOX_IN_H = 0xC280000,
		DSP_MAILBOX_IN_L = 0xC280002,
		DSP_MAILBOX_OUT_H = 0xC280004,
		DSP_MAILBOX_OUT_L = 0xC280006,
		DSP_CONTROL_STATUS = 0xC28000A
	};
	
	DSPController(Emulator *physmem);
	
	void reset();
	void update();
	
	uint16_t read(uint32_t addr);
	void write(uint32_t addr, uint16_t value);
	
	bool check_interrupts();

private:
	DSPInterpreter *interpreter;
	
	bool int_status;
	bool int_enabled;
};
