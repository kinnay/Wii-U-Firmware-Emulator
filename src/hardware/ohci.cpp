
#include "hardware/ohci.h"
#include "hardware.h"
#include "physicalmemory.h"


void OHCIPort::reset() {
	device_connected = true;
	port_enabled = false;
	port_suspended = false;
	port_power = false;
	low_speed = false;
	connect_change = false;
	enable_change = false;
	suspend_change = false;
	reset_change = false;
}

uint32_t OHCIPort::read_status() {
	uint32_t value = 0;
	if (device_connected) value |= 1;
	if (port_enabled) value |= 2;
	if (port_suspended) value |= 4;
	if (port_power) value |= 0x100;
	if (low_speed) value |= 0x200;
	if (connect_change) value |= 0x10000;
	if (enable_change) value |= 0x20000;
	if (suspend_change) value |= 0x40000;
	if (reset_change) value |= 0x100000;
	return value;
}

void OHCIPort::write_status(uint32_t value) {
	if (value & 1) port_enabled = false;
	if (value & 2) {
		if (device_connected) port_enabled = true;
		else connect_change = true;
	}
	if (value & 4) {
		if (device_connected) port_suspended = true;
		else connect_change = true;
	}
	if (value & 8) port_suspended = false;
	if (value & 0x10) {
		if (device_connected) reset_change = true;
		else connect_change = true;
	}
	if (value & 0x100) port_power = true;
	if (value & 0x200) port_power = false;
	if (value & 0x10000) connect_change = false;
	if (value & 0x20000) enable_change = false;
	if (value & 0x40000) suspend_change = false;
	if (value & 0x100000) reset_change = false;
}


const char *OHCIStateNames[] = {
	"USBReset", "USBResume", "USBOperational", "USBSuspend"
};

OHCIController::OHCIController(Hardware *hardware, PhysicalMemory *physmem, int index) {
	this->hardware = hardware;
	this->physmem = physmem;
	this->index = index;
}

void OHCIController::reset() {
	control = 0;
	interrupt_status = 0;
	interrupt_enable = 0;
	hcca = 0;
	control_head_ed = 0;
	bulk_head_ed = 0;
	done_head = 0;
	fminterval = 0x2EDF;
	fmremaining = 0;
	fmnumber = 0;
	periodic_start = 0;
	descriptor_a = (1 << 24) | 3;
	descriptor_b = 0;
	
	state = USBReset;
	
	for (int i = 0; i < 4; i++) {
		ports[i].reset();
	}
	for (int i = 0; i < 4; i++) {
		devices[i].reset();
	}
}

uint32_t OHCIController::read(uint32_t addr) {
	switch (addr) {
		case HcRevision: return 0x10;
		case HcControl: return control;
		case HcCommandStatus: return 0;
		case HcInterruptStatus: return interrupt_status;
		case HcInterruptEnable: return interrupt_enable;
		case HcControlHeadED: return control_head_ed;
		case HcRhDescriptorA: return descriptor_a;
		case HcRhDescriptorB: return descriptor_b;
		case HcRhStatus: return 0;
	}
	
	if (HcRhPortStatusStart <= addr && addr < HcRhPortStatusEnd) {
		int port = (addr - HcRhPortStatusStart) / 4;
		return ports[port].read_status();
	}
	
	Logger::warning("Unknown ohci read: 0x%X", addr);
	return 0;
}

void OHCIController::write(uint32_t addr, uint32_t value) {
	if (addr == HcControl) {
		control = value;
		
		State state = (State)((value >> 6) & 3);
		if (state != this->state) {
			Logger::info("OHCI state set to %s", OHCIStateNames[state]);
			if (state == USBOperational) {
				fmremaining = fminterval;
			}
			this->state = state;
		}
	}
	else if (addr == HcCommandStatus) {
		if (value & 1) reset();
		if (value & 2) process_control();
		if (value & 4) process_bulk();
	}
	else if (addr == HcInterruptStatus) interrupt_status &= ~value;
	else if (addr == HcInterruptEnable) {
		interrupt_enable |= value;
	}
	else if (addr == HcInterruptDisable) interrupt_enable &= ~value;
	else if (addr == HcHCCA) hcca = value;
	else if (addr == HcControlHeadED) control_head_ed = value;
	else if (addr == HcBulkHeadED) bulk_head_ed = value;
	else if (addr == HcFmInterval) fminterval = value & 0x3FFF;
	else if (addr == HcPeriodicStart) periodic_start = value;
	else if (addr == HcRhDescriptorA) descriptor_a = value;
	else if (addr == HcRhDescriptorB) descriptor_b = value;
	else if (addr == HcRhStatus) {
		if (value & 1) {
			for (int i = 0; i < 4; i++) ports[i].port_power = false;
		}
		if (value & 0x10000) {
			for (int i = 0; i < 4; i++) ports[i].port_power = true;
		}
	}
	else if (HcRhPortStatusStart <= addr && addr < HcRhPortStatusEnd) {
		int port = (addr - HcRhPortStatusStart) / 4;
		ports[port].write_status(value);
	}
	else {
		Logger::warning("Unknown ohci write: 0x%X (0x%08X)", addr, value);
	}
}

void OHCIController::update() {
	if (state == USBOperational) {
		if (fmremaining-- == 0) {
			fmremaining = fminterval;
			fmnumber++;
			
			physmem->write(hcca + 0x80, &fmnumber, 2);
			
			interrupt_status |= 4;
			if ((fmnumber & ~0x7FFF) == 0) {
				interrupt_status |= 0x20;
			}
			
			process_periodic();
		}
		
		if (interrupt_enable & interrupt_status) {
			if (index == 0) hardware->trigger_irq_all(Hardware::ARM, 5);
			else if (index == 1) hardware->trigger_irq_all(Hardware::ARM, 6);
			else if (index == 2) hardware->trigger_irq_lt(Hardware::ARM, 3);
			else if (index == 3) hardware->trigger_irq_lt(Hardware::ARM, 5);
		}
	}
}

void OHCIController::process_periodic() {
	if (control & 4) {
		Logger::warning("OHCI periodic list processing not implemented");
	}
}

void OHCIController::process_control() {
	if (control & 0x10) {
		process_eds(control_head_ed);
	}
}

void OHCIController::process_bulk() {
	if (control & 0x20) {
		process_eds(bulk_head_ed);
	}
}

void OHCIController::process_eds(uint32_t current_ed) {
	while (current_ed) {
		OHCIEndpointDescriptor ed;
		physmem->read(current_ed, &ed, 16);
		
		int function = ed.control & 0x7F;
		if (!(ed.control & 0x4000)) {
			process_tds(ed.head & ~0xF, ed.tail & ~0xF, function);
			ed.head = ed.tail;
			physmem->write(current_ed, &ed, 16);
		}
		
		current_ed = ed.next & ~0xF;
	}
	
	physmem->write(hcca + 0x84, &done_head, 4);
	done_head = 0;
	
	interrupt_status |= 2;
}

void OHCIController::process_tds(uint32_t head, uint32_t tail, int function) {
	USBDevice *device = get_device(function);
	if (!device) {
		Logger::error("Invalid USB device address: %i", function);
		return;
	}
	
	while (head != tail) {
		OHCITransferDescriptor td;
		physmem->read(head, &td, 16);
		
		uint32_t size = td.end - td.buffer + 1;
		
		int direction = (td.control >> 19) & 3;
		if (direction == 0) {
			Buffer data = physmem->read(td.buffer, size);
			device->setup(data);
		}
		else if (direction == 1) {
			Buffer data = physmem->read(td.buffer, size);
			device->write(data);
		}
		else if (direction == 2) {
			Buffer data = device->read(size);
			physmem->write(td.buffer, data);
		}
		
		uint32_t next = td.next;
		
		td.control &= ~0xFC000000;
		td.buffer = 0;
		td.next = done_head;
		done_head = head;
		
		physmem->write(head, &td, 16);
		
		head = next;
	}
}

USBDevice *OHCIController::get_device(int address) {
	for (int i = 0; i < 4; i++) {
		if (devices[i].address == address) {
			return &devices[i];
		}
	}
	return nullptr;
}
