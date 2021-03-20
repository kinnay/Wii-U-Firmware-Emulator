
#pragma once

#include <mutex>

#include <cstdint>


class IPCController {
public:
	enum Register {
		LT_IPC_PPCMSG = 0,
		LT_IPC_PPCCTRL = 4,
		LT_IPC_ARMMSG = 8,
		LT_IPC_ARMCTRL = 0xC
	};
	
	void reset();
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
	bool check_interrupts_arm();
	bool check_interrupts_ppc();
	
private:
	std::mutex mutex;
	
	uint32_t ppcmsg;
	uint32_t armmsg;
	
	bool x1, x2, y1, y2;
	bool ix1, ix2, iy1, iy2;
};
