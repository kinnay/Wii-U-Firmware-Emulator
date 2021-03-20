
#include "hardware/ehci.h"

#include "common/logger.h"


void EHCIPort::reset() {
	status = 0;
}

uint32_t EHCIPort::read_status() {
	return status;
}

void EHCIPort::write_status(uint32_t value) {
	status = value;
}


void EHCIController::reset() {
	usbcmd = 0x80000;
	usbsts = 0x1000;
	usbintr = 0;
	frindex = 0;
	ctrlsegment = 0;
	periodiclist = 0;
	asynclist = 0;
	configflag = 0;
	
	a4 = 0;
	
	for (int i = 0; i < 6; i++) {
		ports[i].reset();
	}
}

uint32_t EHCIController::read(uint32_t addr) {
	switch (addr) {
		case USBCMD: return usbcmd;
		case USBSTS: return usbsts;
		case USBINTR: return usbintr;
		case FRINDEX: return frindex;
		case CTRLDSSEGMENT: return ctrlsegment;
		case EHCI_A4: return a4;
	}
	
	if (PORTSC_START <= addr && addr < PORTSC_END) {
		int port = (addr - PORTSC_START) / 4;
		return ports[port].read_status();
	}
	
	Logger::warning("Unknown ehci read: 0x%X", addr);
	return 0;
}

void EHCIController::write(uint32_t addr, uint32_t value) {
	if (addr == USBCMD) {
		usbcmd = value & ~3;
		if (value & 2) {
			reset();
		}
		if (value & 1) {
			Logger::warning("EHCI command processing not implemented");
		}
	}
	else if (addr == USBSTS) usbsts &= ~value;
	else if (addr == USBINTR) usbintr = value;
	else if (addr == FRINDEX) frindex = value;
	else if (addr == PERIODICLISTBASE) periodiclist = value;
	else if (addr == ASYNCLISTADDR) asynclist = value;
	else if (addr == CONFIGFLAG) configflag = value;
	else if (addr == EHCI_A4) a4 = value;
	else if (addr == EHCI_B0) {}
	else if (addr == EHCI_B4) {}
	else if (addr == EHCI_CC) {}
	else if (PORTSC_START <= addr && addr < PORTSC_END) {
		int port = (addr - PORTSC_START) / 4;
		ports[port].write_status(value);
	}
	else {
		Logger::warning("Unknown ehci write: 0x%X (0x%08X)", addr, value);
	}
}
