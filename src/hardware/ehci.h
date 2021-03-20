
#pragma once

#include <cstdint>


class EHCIPort {
public:
	void reset();
	
	uint32_t read_status();
	void write_status(uint32_t value);
	
private:
	uint32_t status;
};


class EHCIController {
public:
	enum Register {
		USBCMD = 0,
		USBSTS = 4,
		USBINTR = 8,
		FRINDEX = 0xC,
		CTRLDSSEGMENT = 0x10,
		PERIODICLISTBASE = 0x14,
		ASYNCLISTADDR = 0x18,
		CONFIGFLAG = 0x40,
		
		PORTSC_START = 0x54,
		PORTSC_END = 0x6C,
		
		EHCI_A4 = 0xA4,
		EHCI_B0 = 0xB0,
		EHCI_B4 = 0xB4,
		EHCI_CC = 0xCC
	};
	
	void reset();
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
private:
	uint32_t usbcmd;
	uint32_t usbsts;
	uint32_t usbintr;
	uint32_t frindex;
	uint32_t ctrlsegment;
	uint32_t periodiclist;
	uint32_t asynclist;
	uint32_t configflag;
	uint32_t a4;
	
	EHCIPort ports[6];
};
