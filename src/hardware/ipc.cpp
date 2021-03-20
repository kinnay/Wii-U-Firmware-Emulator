
#include "hardware/ipc.h"

#include "common/logger.h"


void IPCController::reset() {
	ppcmsg = 0;
	armmsg = 0;
	
	x1 = false;
	x2 = false;
	y1 = false;
	y2 = false;
	ix1 = false;
	ix2 = false;
	iy1 = false;
	iy2 = false;
}

uint32_t IPCController::read(uint32_t addr) {
	std::lock_guard<std::mutex> guard(mutex);
	
	switch (addr) {
		case LT_IPC_PPCMSG: return ppcmsg;
		case LT_IPC_PPCCTRL: return x1 | (y2 << 1) | (y1 << 2) | (x2 << 3) | (iy1 << 4) | (iy2 << 5);
		case LT_IPC_ARMMSG: return armmsg;
		case LT_IPC_ARMCTRL: return y1 | (x2 << 1) | (x1 << 2) | (y2 << 3) | (ix1 << 4) | (ix2 << 5);
	}
	
	Logger::warning("Unknown ipc read: 0x%X", addr);
	return 0;
}

void IPCController::write(uint32_t addr, uint32_t value) {
	std::lock_guard<std::mutex> guard(mutex);
	if (addr == LT_IPC_PPCMSG) ppcmsg = value;
	else if (addr == LT_IPC_PPCCTRL) {
		if (value & 1) x1 = true;
		if (value & 2) y2 = false;
		if (value & 4) y1 = false;
		if (value & 8) x2 = true;
		iy1 = value & 0x10;
		iy2 = value & 0x20;
	}
	else if (addr == LT_IPC_ARMMSG) armmsg = value;
	else if (addr == LT_IPC_ARMCTRL) {
		if (value & 1) y1 = true;
		if (value & 2) x2 = false;
		if (value & 4) x1 = false;
		if (value & 8) y2 = true;
		ix1 = value & 0x10;
		ix2 = value & 0x20;
	}
	else {
		Logger::warning("Unknown ipc write: 0x%X (0x%08X)", addr, value);
	}
}

bool IPCController::check_interrupts_arm() {
	return (x1 && ix1) || (x2 && ix2);
}

bool IPCController::check_interrupts_ppc() {
	return (y1 && iy1) || (y2 && iy2);
}
