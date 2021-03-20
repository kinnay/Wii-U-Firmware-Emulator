
#pragma once

#include "cpu/ppc/ppcreservation.h"
#include "cpu/ppc/ppcprocessor.h"
#include "cpu/arm/armprocessor.h"

#include "debugger/debugger.h"

#include "physicalmemory.h"
#include "hardware.h"


class Emulator {
public:
	Emulator();
	
	void run();
	void pause();
	void signal(int core);
	void reset();
	void quit();
	
	PhysicalMemory physmem;
	
	PPCReservation reservation;
	PPCProcessor ppc[3];
	ARMProcessor arm;
	
	Hardware hardware;
	Debugger debugger;

private:	
	int core;
	
	bool running;
};
