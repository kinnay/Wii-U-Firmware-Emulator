
#include "emulator.h"
#include "processor.h"
#include "config.h"

#include <algorithm>


Processor::Processor(Emulator *emulator, int index, bool threaded) {
	this->emulator = emulator;
	this->physmem = &emulator->physmem;
	this->hardware = &emulator->hardware;
	this->threaded = threaded;
	this->index = index;
	enabled = false;
	paused = true;
}

void Processor::start() {
	paused = false;
	if (threaded && enabled) {
		thread = std::thread(threadFunc, this);
	}
}

void Processor::pause() {
	paused = true;
	if (threaded && thread.joinable()) {
		thread.join();
	}
}

void Processor::enable() {
	enabled = true;
	if (threaded && !paused) {
		thread = std::thread(threadFunc, this);
	}
}

void Processor::disable() {
	enabled = false;
	if (threaded && thread.joinable()) {
		thread.join();
	}
	reset();
}

bool Processor::isPaused() {
	return paused;
}

bool Processor::isEnabled() {
	return enabled;
}

void Processor::setEnabled(bool enabled) {
	if (enabled) enable();
	else disable();
}

void Processor::threadFunc(Processor *cpu) {
	cpu->mainLoop();
}

void Processor::mainLoop() {
	while (!paused && enabled) {
		step();
	}
}

#if BREAKPOINTS
void Processor::checkBreakpoints(uint32_t pc) {
	if (isBreakpoint(pc)) {
		Sys::out->write("Breakpoint hit at 0x%X\n", pc);
		
		emulator->signal(index);
		paused = true;
	}
}

bool Processor::isBreakpoint(uint32_t addr) {
	for (uint32_t bp : breakpoints) {
		if (bp == addr) {
			return true;
		}
	}
	return false;
}

void Processor::addBreakpoint(uint32_t addr) {
	breakpoints.push_back(addr);
}

void Processor::removeBreakpoint(uint32_t addr) {
	breakpoints.erase(std::remove(breakpoints.begin(), breakpoints.end(), addr), breakpoints.end());
}
#endif

#if WATCHPOINTS
void Processor::checkWatchpoints(bool write, bool virt, uint32_t addr, int length) {
	if (isWatchpoint(write, virt, addr, length)) {
		Sys::out->write(
			"Watchpoint (%s) hit at %s address 0x%X\n",
			write ? "write" : "read",
			virt ? "virtual" : "physical",
			addr
		);
		
		emulator->signal(index);
		paused = true;
	}
}

bool Processor::isWatchpoint(bool write, bool virt, uint32_t addr, int length) {
	for (uint32_t wp : watchpoints[write][virt]) {
		if (addr <= wp && wp < addr + length) {
			return true;
		}
	}
	return false;
}

void Processor::addWatchpoint(bool write, bool virt, uint32_t addr) {
	watchpoints[write][virt].push_back(addr);
}

void Processor::removeWatchpoint(bool write, bool virt, uint32_t addr) {
	watchpoints[write][virt].erase(
		std::remove(watchpoints[write][virt].begin(), watchpoints[write][virt].end(), addr),
		watchpoints[write][virt].end()
	);
}
#endif
