
#include "class_interpreter.h"
#include "class_endian.h"
#include "errors.h"

Interpreter::Interpreter(IPhysicalMemory *physmem, IVirtualMemory *virtmem, bool bigEndian)
	: physmem(physmem), virtmem(virtmem), watchpointHit(false) {
	swapEndian = bigEndian != (Endian::getSystemEndian() == Endian::Big);
}

void Interpreter::checkWatchpoints(bool write, uint32_t addr, uint32_t length) {
	auto &watchpoints = write ? watchpointsWrite : watchpointsRead;
	
	for (uint32_t watchpoint : watchpoints) {
		if (addr <= watchpoint && watchpoint < addr + length) {
			watchpointHit = true;
			watchpointAddr = watchpoint;
			watchpointWrite = write;
		}
	}
}

void Interpreter::setDataErrorCB(DataErrorCB callback) { dataErrorCB = callback; }
void Interpreter::setFetchErrorCB(FetchErrorCB callback) { fetchErrorCB = callback; }
void Interpreter::setBreakpointCB(BreakpointCB callback) { breakpointCB = callback; }
void Interpreter::setWatchpointCB(bool write, WatchpointCB callback) {
	if (write) watchpointWriteCB = callback;
	else watchpointReadCB = callback;
}

void Interpreter::setAlarm(uint32_t alarm, AlarmCB callback) {
	alarmInterval = alarmTimer = alarm;
	alarmCB = callback;
}

void Interpreter::invalidateMMUCache() {
	virtmem->invalidateCache();
}

void Interpreter::handleMemoryError(uint32_t addr, bool isWrite, bool isCode) {
	if (isCode) {
		if (fetchErrorCB) fetchErrorCB(addr);
	}
	if (dataErrorCB) dataErrorCB(addr, isWrite);
}

bool Interpreter::run(int steps) {
	while (true) {
		if (!step()) {
			//If a memory access fails step() returns false, but if the exception
			//callback handles it without throwing a new error we want to continue
			//execution
			if (ErrorOccurred()) return false;
		}
		
		if (alarmCB && --alarmTimer == 0) {
			alarmTimer = alarmInterval;
			if (!alarmCB()) return false;
		}
		
		if (steps) {
			if (--steps == 0) {
				return true;
			}
		}
		
		#ifdef WATCHPOINTS
		if (watchpointHit) {
			watchpointHit = false;
			if (watchpointWrite) {
				if (!watchpointWriteCB) {
					RuntimeError("No watchpoint handler installed (write)");
					return false;
				}
				if (!watchpointWriteCB(watchpointAddr, true)) return false;
			}
			else {
				if (!watchpointReadCB) {
					RuntimeError("No watchpoint handler installed (read)");
					return false;
				}
				if (!watchpointReadCB(watchpointAddr, false)) return false;
			}
		}
		#endif
		
		#ifdef BREAKPOINTS
		if (std::find(breakpoints.begin(), breakpoints.end(), getPC()) != breakpoints.end()) {
			if (!breakpointCB) {
				RuntimeError("No breakpoint handler installed");
				return false;
			}
			if (!breakpointCB(getPC())) {
				return false;
			}
		}
		#endif
	}
	return true;
}
