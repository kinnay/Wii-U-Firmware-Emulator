
#pragma once

#include "interface_physmem.h"
#include "interface_virtmem.h"
#include <functional>
#include <vector>
#include <cstdint>

class Interpreter {
	public:
	typedef std::function<bool(uint32_t addr, bool isWrite)> DataErrorCB;
	typedef std::function<bool(uint32_t addr)> FetchErrorCB;
	typedef std::function<bool(uint32_t addr)> BreakpointCB;
	typedef std::function<bool(uint32_t addr)> WatchpointCB;
	typedef std::function<bool()> AlarmCB;
	
	Interpreter(IPhysicalMemory *physmem, IVirtualMemory *virtmem, bool bigEndian);
	
	template <class T>
	bool read(uint32_t addr, T *value, bool isCode=false) {
		#ifdef WATCHPOINTS
		if (!isCode) {
			checkWatchpoints(false, addr, sizeof(T));
		}
		#endif

		IVirtualMemory::Access type = isCode ? IVirtualMemory::Instruction : IVirtualMemory::DataRead;
		if (!virtmem->translate(&addr, sizeof(T), type)) {
			return handleMemoryError(addr, false, isCode);
		}

		int result = physmem->read<sizeof(T)>(addr, value);
		if (result == -1) return false;
		else if (result == -2) {
			return handleMemoryError(addr, false, isCode);
		}
		if (swapEndian) Endian::swap<sizeof(T)>(value);
		return true;
	}

	template <class T>
	bool write(uint32_t addr, T value) {
		#ifdef WATCHPOINTS
		checkWatchpoints(true, addr, sizeof(T));
		#endif
		
		if (swapEndian) Endian::swap<sizeof(T)>(&value);
		if (!virtmem->translate(&addr, sizeof(T), IVirtualMemory::DataWrite)) {
			return handleMemoryError(addr, true, false);
		}
		
		int result = physmem->write<sizeof(T)>(addr, &value);
		if (result == -1) return false;
		else if (result == -2) {
			return handleMemoryError(addr, true, false);
		}
		return true;
	}
	
	bool run(int steps = 0);
	
	virtual bool step() = 0;
	virtual uint32_t getPC() = 0;
	
	void setDataErrorCB(DataErrorCB callback);
	void setFetchErrorCB(FetchErrorCB callback);
	void setBreakpointCB(BreakpointCB callback);
	void setWatchpointCB(bool write, WatchpointCB callback);
	void setAlarm(uint32_t interval, AlarmCB callback);
	
	std::vector<uint32_t> breakpoints;
	std::vector<uint32_t> watchpointsRead;
	std::vector<uint32_t> watchpointsWrite;
	
	private:
	IPhysicalMemory *physmem;
	IVirtualMemory *virtmem;
	
	DataErrorCB dataErrorCB;
	FetchErrorCB fetchErrorCB;
	
	BreakpointCB breakpointCB;
	
	bool watchpointHit;
	bool watchpointWrite;
	uint32_t watchpointAddr;
	WatchpointCB watchpointReadCB;
	WatchpointCB watchpointWriteCB;
	
	uint32_t alarmTimer;
	uint32_t alarmInterval;
	AlarmCB alarmCB;
	
	bool swapEndian;
	
	void checkWatchpoints(bool write, uint32_t addr, uint32_t length);
	bool handleMemoryError(uint32_t addr, bool isWrite, bool isCode);
};
