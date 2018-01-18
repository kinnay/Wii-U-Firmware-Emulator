
#pragma once

#include "interface_physmem.h"
#include "interface_virtmem.h"
#include "class_endian.h"
#include "errors.h"

#include <functional>
#include <vector>
#include <cstdint>

const int iCacheSize = 0x30;

class Interpreter {
	public:
	typedef std::function<bool(uint32_t addr, bool isWrite)> DataErrorCB;
	typedef std::function<bool(uint32_t addr)> FetchErrorCB;
	typedef std::function<bool(uint32_t addr)> BreakpointCB;
	typedef std::function<bool(uint32_t addr, bool isWrite)> WatchpointCB;
	typedef std::function<bool()> AlarmCB;
	
	Interpreter(IPhysicalMemory *physmem, IVirtualMemory *virtmem, bool bigEndian);
	
	template <class T>
	bool readCode(uint32_t addr, T *value) {
		if (iCacheEnabled) {
			if (iCacheValid && iCacheStart <= addr && addr < iCacheStart + iCacheSize) {
				//Cache hit
				*value = *(T *)(&instructionCache[addr - iCacheStart]);
				if (swapEndian) Endian::swap(value);
				return true;
			}
			
			else {
				//Cache miss
				if (virtmem->translate(&addr, iCacheSize, IVirtualMemory::Instruction)) {
					if (physmem->read(addr, instructionCache, iCacheSize) == 0) {
						iCacheStart = addr;
						iCacheValid = true;
						*value = *(T *)(&instructionCache[addr - iCacheStart]);
						if (swapEndian) Endian::swap(value);
						return true;
					}
				}
				ErrorClear();
			}
		}
		
		if (!virtmem->translate(&addr, sizeof(T), IVirtualMemory::Instruction)) {
			handleMemoryError(addr, false, true);
			return false;
		}
		
		int result = physmem->read(addr, value);
		if (result == -1) return false;
		else if (result == -2) {
			handleMemoryError(addr, false, true);
			return false;
		}
		
		if (swapEndian) Endian::swap(value);
		return true;
	}
	
	template <class T>
	bool read(uint32_t addr, T *value) {
		#ifdef WATCHPOINTS
		checkWatchpoints(false, addr, sizeof(T));
		#endif

		if (!virtmem->translate(&addr, sizeof(T), IVirtualMemory::DataRead)) {
			handleMemoryError(addr, false, false);
			return false;
		}

		int result = physmem->read<sizeof(T)>(addr, value);
		if (result == -1) return false;
		else if (result == -2) {
			handleMemoryError(addr, false, false);
			return false;
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
			handleMemoryError(addr, true, false);
			return false;
		}
		
		int result = physmem->write<sizeof(T)>(addr, &value);
		if (result == -1) return false;
		else if (result == -2) {
			handleMemoryError(addr, true, false);
			return false;
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
	
	void setICacheEnabled(bool enabled);
	void invalidateICache();
	void invalidateMMUCache();
	
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
	
	uint8_t instructionCache[iCacheSize];
	uint32_t iCacheStart;
	bool iCacheEnabled;
	bool iCacheValid;
	
	bool swapEndian;
	
	void checkWatchpoints(bool write, uint32_t addr, uint32_t length);
	void handleMemoryError(uint32_t addr, bool isWrite, bool isCode);
};
