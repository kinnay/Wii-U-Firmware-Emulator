
#pragma once

#include "cpu/ppc/ppccore.h"
#include "cpu/ppc/ppcmetrics.h"
#include "cpu/ppc/ppcreservation.h"
#include "cpu/ppc/ppccodegenerator.h"
#include "cpu/ppc/ppcmmu.h"
#include "cpu/processor.h"
#include "cpu/jit.h"

#include "hardware/pi.h"
#include "logger.h"
#include "config.h"
#include "enum.h"


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
	
	void copy(uint32_t dst, uint32_t src, uint32_t size);
	
	void step();
	void reset();
	
	PPCCore core;
	PPCMMU mmu;
	PPCReservation *reservation;
	
	JIT<PPCCodeGenerator, uint32_t> jit;
	
	#if STATS
	uint64_t instrsExecuted;
	#endif
	
	#if METRICS
	PPCMetrics metrics;
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

