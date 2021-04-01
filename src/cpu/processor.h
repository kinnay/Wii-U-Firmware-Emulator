
#pragma once

#include "config.h"

#include <vector>
#include <thread>
#include <cstdint>


class Emulator;
class PhysicalMemory;
class Hardware;


class Processor {
public:
	Processor(Emulator *emulator, int index, bool threaded);
	
	void start();
	void pause();
	
	void enable();
	void disable();
	
	bool isPaused();
	bool isEnabled();
	
	void setEnabled(bool enabled);
	
	virtual void reset() = 0;
	virtual void step() = 0;

	#if BREAKPOINTS
	bool isBreakpoint(uint32_t addr);
	void addBreakpoint(uint32_t addr);
	void removeBreakpoint(uint32_t addr);
	
	std::vector<uint32_t> breakpoints;
	#endif
	
	#if WATCHPOINTS
	bool isWatchpoint(bool write, bool virt, uint32_t addr, int size);
	void addWatchpoint(bool write, bool virt, uint32_t addr);
	void removeWatchpoint(bool write, bool virt, uint32_t addr);
	
	std::vector<uint32_t> watchpoints[2][2];
	#endif

protected:
	Emulator *emulator;
	PhysicalMemory *physmem;
	Hardware *hardware;
	int index;
	
	#if BREAKPOINTS
	void checkBreakpoints(uint32_t addr);
	#endif
	
	#if WATCHPOINTS
	void checkWatchpoints(bool write, bool virt, uint32_t addr, int size);
	#endif

private:
	static void threadFunc(Processor *cpu);
	
	void mainLoop();
	
	std::thread thread;
	bool threaded;
	bool enabled;
	bool paused;
};
