
#pragma once

#include "hardware/usb.h"

#include <cstdint>


class Hardware;
class PhysicalMemory;


struct OHCIEndpointDescriptor {
	uint32_t control;
	uint32_t tail;
	uint32_t head;
	uint32_t next;
};

struct OHCITransferDescriptor {
	uint32_t control;
	uint32_t buffer;
	uint32_t next;
	uint32_t end;
};


class OHCIPort {
public:
	void reset();
	
	uint32_t read_status();
	void write_status(uint32_t value);
	
	bool device_connected;
	bool port_enabled;
	bool port_suspended;
	bool port_power;
	bool low_speed;
	bool connect_change;
	bool enable_change;
	bool suspend_change;
	bool reset_change;
};


class OHCIController {
public:
	enum Register {
		HcRevision = 0,
		HcControl = 4,
		HcCommandStatus = 8,
		HcInterruptStatus = 0xC,
		HcInterruptEnable = 0x10,
		HcInterruptDisable = 0x14,
		HcHCCA = 0x18,
		HcControlHeadED = 0x20,
		HcBulkHeadED = 0x28,
		HcFmInterval = 0x34,
		HcFmRemaining = 0x38,
		HcFmNumber = 0x3C,
		HcPeriodicStart = 0x40,
		HcRhDescriptorA = 0x48,
		HcRhDescriptorB = 0x4C,
		HcRhStatus = 0x50,
		
		HcRhPortStatusStart = 0x54,
		HcRhPortStatusEnd = 0x64
	};
	
	enum State {
		USBReset, USBResume, USBOperational, USBSuspend
	};
	
	OHCIController(Hardware *hardware, PhysicalMemory *physmem, int index);
	
	void reset();
	void update();
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
private:
	void process_periodic();
	void process_control();
	void process_bulk();
	
	void process_eds(uint32_t current_ed);
	void process_tds(uint32_t head, uint32_t tail, int function);
	
	USBDevice *get_device(int address);
	
	uint32_t control;
	uint32_t interrupt_status;
	uint32_t interrupt_enable;
	uint32_t hcca;
	uint32_t control_head_ed;
	uint32_t bulk_head_ed;
	uint32_t done_head;
	uint16_t fminterval;
	uint16_t fmremaining;
	uint16_t fmnumber;
	uint16_t periodic_start;
	uint32_t descriptor_a;
	uint32_t descriptor_b;
	
	State state;
	
	OHCIPort ports[4];
	USBDummyDevice devices[4];
	
	int index;
	Hardware *hardware;
	PhysicalMemory *physmem;
};
