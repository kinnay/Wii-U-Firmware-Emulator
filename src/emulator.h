
#pragma once

#include "ppcprocessor.h"
#include "ppcreservation.h"
#include "armprocessor.h"
#include "physicalmemory.h"
#include "hardware.h"
#include "debugger.h"


class Emulator {
public:
	Emulator();
	
	void run();
	void pause();
	void signal(int core);
	void reset();
	void quit();
	
	Hardware hardware;
	PhysicalMemory physmem;
	
	ARMProcessor armcpu;
	PPCProcessor ppc[3];
	
private:
	PPCReservation reservation;
	Debugger debugger;
	int core;
	
	bool running;
};
