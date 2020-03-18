
#pragma once

#include "hardware/pi.h"
#include "ppccore.h"
#include "ppcjit.h"
#include "ppcmetrics.h"
#include "ppcreservation.h"
#include "processor.h"
#include "ppcmmu.h"
#include "logger.h"
#include "enum.h"
#include "config.h"


class Emulator;

class PPCProcessor : public Processor {
public:
	PPCProcessor(Emulator *emulator, PPCReservation *reservation, int index);
	
	template <class T>
	bool read(uint32_t addr, T *value) {
		#if WATCHPOINTS
		checkWatchpoints(false, true, addr, sizeof(T));
		#endif
		
		bool supervisor = !(core.msr & 0x4000);
		if (!mmu.translate(&addr, MemoryAccess::DataRead, supervisor)) {
			core.sprs[PPCCore::DAR] = addr;
			core.sprs[PPCCore::DSISR] = 0x40000000;
			core.triggerException(PPCCore::DSI);
			return false;
		}
		
		#if WATCHPOINTS
		checkWatchpoints(false, false, addr, sizeof(T));
		#endif
		
		*value = physmem->read<T>(addr);
		return true;
	}
	
	template <class T>
	bool write(uint32_t addr, T value) {
		#if WATCHPOINTS
		checkWatchpoints(true, true, addr, sizeof(T));
		#endif
		
		reservation->write(addr);
		
		bool supervisor = !(core.msr & 0x4000);
		if (!mmu.translate(&addr, MemoryAccess::DataWrite, supervisor)) {
			core.sprs[PPCCore::DAR] = addr;
			core.sprs[PPCCore::DSISR] = 0x42000000;
			core.triggerException(PPCCore::DSI);
			return false;
		}
		
		#if WATCHPOINTS
		checkWatchpoints(true, false, addr, sizeof(T));
		#endif
		
		if (addr == (core.sprs[PPCCore::WPAR] & ~0x1F)) {
			if (core.sprs[PPCCore::HID2] & 0x40000000) {
				wg->write_data(value);
				return true;
			}
		}
		
		physmem->write<T>(addr, value);
		return true;
	}
	
	void step();
	void reset();
	
	PPCCore core;
	PPCMMU mmu;
	PPCReservation *reservation;
	PPCJIT jit;
	
	#if METRICS
	PPCMetrics metrics;
	#endif
	
	#if STATS
	uint64_t instrsExecuted;
	#endif
	
private:
	void updateTimer();
	void checkDebugPoints();
	void checkInterrupts();
	
	int timer;
	
	WGController *wg;
	
	ConsoleLogger printer;
	FileLogger coslog;
};

