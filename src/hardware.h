
#pragma once

#include "hardware/aes.h"
#include "hardware/ahci.h"
#include "hardware/ahmn.h"
#include "hardware/ai.h"
#include "hardware/dsp.h"
#include "hardware/ehci.h"
#include "hardware/exi.h"
#include "hardware/gpu.h"
#include "hardware/latte.h"
#include "hardware/mem.h"
#include "hardware/nand.h"
#include "hardware/ohci.h"
#include "hardware/pi.h"
#include "hardware/sdio.h"
#include "hardware/sha.h"

#include "common/logger.h"


class Emulator;

class Hardware {
public:
	Hardware(Emulator *emulator);
	
	template <class T>
	T read(uint32_t addr) {
		Logger::warning("Unknown physical memory read: 0x%08X", addr);
		return 0;
	}
	
	template <class T>
	void write(uint32_t addr, T value) {
		Logger::warning("Unknown physical memory write: 0x%08X", addr);
	}
	
	void reset();
	void update();
	
	bool check_interrupts_arm();
	bool check_interrupts_ppc(int core);

	LatteController latte;
	PIController pi;
	
	EXIController exi;
	AHMNController ahmn;
	MEMController mem;
	AESController aes;
	SHAController sha;
	NANDController nand;
	GPUController gpu;
	AIController ai;
	DSPController dsp;
	
	AHCIController ahci;
	
	EHCIController ehci0;
	EHCIController ehci1;
	EHCIController ehci2;
	
	OHCIController ohci00;
	OHCIController ohci01;
	OHCIController ohci1;
	OHCIController ohci2;
	
	SDIOController sdio0;
	SDIOController sdio1;
	SDIOController sdio2;
	SDIOController sdio3;
};


template <> uint16_t Hardware::read<uint16_t>(uint32_t addr);
template <> uint32_t Hardware::read<uint32_t>(uint32_t addr);

template <> void Hardware::write<uint16_t>(uint32_t addr, uint16_t value);
template <> void Hardware::write<uint32_t>(uint32_t addr, uint32_t value);
