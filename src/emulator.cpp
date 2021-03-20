
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


Emulator::Emulator() :
	physmem(&hardware),
	hardware(this),
	debugger(this),
	arm(this),
	ppc {
		{this, &reservation, 0},
		{this, &reservation, 1},
		{this, &reservation, 2}
	}
{
	reset();
}

void Emulator::reset() {
	Buffer buffer = FileUtils::load("files/boot1.bin");
	physmem.write(0xD400200, buffer);
	
	arm.reset();
	arm.core.regs[ARMCore::PC] = 0xD400200;
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
}

void Emulator::signal(int core) {
	this->core = core;
}

void Emulator::quit() {
	running = false;
}
