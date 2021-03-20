
#pragma once

#include <cstdint>

class DSPController {
public:
	enum Register {
		DSP_MAILBOX_IN_H = 0xC280000,
		DSP_MAILBOX_IN_L = 0xC280002,
		DSP_MAILBOX_OUT_H = 0xC280004,
		DSP_MAILBOX_OUT_L = 0xC280006,
		DSP_CONTROL_STATUS = 0xC28000A
	};
	
	void reset();
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
	bool check_interrupts();
	
private:
	enum MessageOut {
		DSP_INIT = 0xDCD10000
	};
	
	enum State {
		STATE_NEXT,
		STATE_ARGUMENT
	};
	
	void accept(uint32_t mail);
	void process(uint32_t message, uint32_t arg);
	
	void send_mail(MessageOut message);
	
	uint32_t mailbox_in;
	uint32_t mailbox_out;
	
	bool halted;
	bool int_status;
	bool int_mask;
	
	uint32_t message;
	State state;
};