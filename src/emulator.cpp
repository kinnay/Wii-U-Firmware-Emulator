
#include "common/fileutils.h"
#include "common/sys.h"
#include "emulator.h"

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
	hardware(&physmem, &ppc[0], &ppc[1], &ppc[2]),
	physmem(&hardware),
	armcpu(this),
	ppc {
		{this, &reservation, 0},
		{this, &reservation, 1},
		{this, &reservation, 2}
	},
	debugger(this)
{
	reset();
}

void Emulator::reset() {
	Buffer buffer = FileUtils::load("files/boot1.bin");
	physmem.write(0x0D400200, buffer.get(), buffer.size());
	
	armcpu.reset();
	armcpu.core.regs[ARMCore::PC] = 0x0D400200;
	armcpu.enable();
	
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
		
		armcpu.start();
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
	armcpu.pause();
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
