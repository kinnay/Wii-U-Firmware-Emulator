
#include "emulator.h"

#include "common/fileutils.h"
#include "common/sys.h"

#include <thread>
#include <chrono>

#include <csignal>


bool keyboard_interrupt;

void signal_handler(int signal) {
	if (signal == SIGINT) {
		keyboard_interrupt = true;
	}
}


Emulator::Emulator(bool boot0) :
	physmem(&hardware),
	hardware(this),
	debugger(this),
	arm(this),
	ppc {
		{this, &reservation, 0},
		{this, &reservation, 1},
		{this, &reservation, 2}
	},
	dsp(this),
	boot0(boot0)
{
	reset();
}

void Emulator::reset() {

	Buffer buffer = FileUtils::load(boot0 ? "files/boot0.bin" : "files/boot1.bin");
	uint32_t load_address = boot0 ? 0xFFFF0000 : 0xD400200;
	physmem.write(load_address, buffer);

	arm.reset();
	arm.core.regs[ARMCore::PC] = load_address;
	arm.enable();
	
	reservation.reset();
	for (int i = 0; i < 3; i++) {
		ppc[i].disable();
		ppc[i].reset();
	}
	
	hardware.reset();
}

void Emulator::run() {
	running = true;
	
	::signal(SIGINT, signal_handler);
	
	debugger.show(0);
	
	while (running) {
		keyboard_interrupt = false;
		core = -1;
		
		arm.start();
		for (int i = 0; i < 3; i++) {
			ppc[i].start();
		}
		dsp.start();
		
		while (core == -1 && !keyboard_interrupt) {
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
		
		if (keyboard_interrupt) {
			Sys::out->write("\n");
			
			debugger.show(-1);
		}
		else {
			debugger.show(core);
		}
	}
}

void Emulator::pause() {
	arm.pause();
	for (int i = 0; i < 3; i++) {
		ppc[i].pause();
	}
	dsp.pause();
}

void Emulator::signal(int core) {
	this->core = core;
}

void Emulator::quit() {
	running = false;
}
