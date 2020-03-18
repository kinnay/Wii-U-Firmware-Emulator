
#include "hardware/i2c.h"
#include "hardware.h"
#include "common/logger.h"
#include <cstring>


void SMCController::reset() {
	system_event_flag = 0;
	usb_control = 0;
	power_failure_state = 0;
	wifi_reset = 0;
	data = 0;
}

void SMCController::read(int slave, uint8_t *data, size_t size) {
	if (slave != 0x50) {
		Logger::warning("Invalid smc slave: %i", slave);
		return;
	}
	
	if (size == 1) {
		data[0] = this->data;
	}
	else {
		Logger::warning("Invalid smc read size: %i", size);
	}
}

void SMCController::write(int slave, Buffer data) {
	if (slave != 0x50) {
		Logger::warning("Invalid smc slave: %i", slave);
		return;
	}
	
	if (data.size() == 1) {
		uint8_t cmd = data.get()[0];
		if (cmd == 0x0) Logger::info("ODD power set to: on");
		else if (cmd == 0x10) Logger::info("ON indicator set to: on");
		else if (cmd == 0x13) Logger::info("ON indicator set to: pulse");
		else if (cmd == 0x1F) Logger::info("OFF indicator set to: pulse");
		else if (cmd == 0x20) Logger::info("BTRST set to: pulse");
		else if (cmd == 0x41) this->data = system_event_flag;
		else if (cmd == 0x43) this->data = usb_control;
		else if (cmd == 0x45) this->data = power_failure_state;
		else if (cmd == 0x46) this->data = wifi_reset;
		else {
			Logger::warning("Unknown smc write: %s", data.hexstring());
		}
	}
	else if (data.size() == 2) {
		uint8_t cmd = data.get()[0];
		uint8_t value = data.get()[1];
		if (cmd == 0x43) usb_control = value;
		else if (cmd == 0x46) wifi_reset = value;
		else {
			Logger::warning("Unknown smc write: %s", data.hexstring());
		}
	}
	else {
		Logger::warning("Unknown smc write: %s", data.hexstring());
	}
}


void HDMIController::reset() {
	memset(interrupt_info, 0, sizeof(interrupt_info));
	
	data = 0;
}

void HDMIController::read(int slave, uint8_t *data, size_t size) {
	if (size == 1) {
		if (slave == 0x38) {
			data[0] = this->data;
		}
		else {
			Logger::warning("Unknown hdmi read slave: %i", slave);
		}
	}
	else {
		Logger::warning("Invalid hdmi read size: %i", size);
	}
}

void HDMIController::write(int slave, Buffer data) {
	if (data.size() == 1) {
		uint8_t cmd = data.get()[0];
		if (slave == 0x38 && cmd >= 0x90 && cmd < 0x97) this->data = interrupt_info[cmd - 0x90];
		else {
			Logger::warning("Unknown hdmi write: %s", data.hexstring());
		}
	}
	else if (data.size() == 2) {
		uint8_t cmd = data.get()[0];
		uint8_t value = data.get()[1];
		if (slave == 0x38 && cmd == 0x07) reset();
		else if (slave == 0x38 && cmd == 0x98) {} // Interrupt control
		else if (slave == 0x3D && cmd == 0x89) {
			interrupt_info[0] |= 0x10;
		}
		else {
			Logger::warning("Unknown hdmi write: %s", data.hexstring());
		}
	}
	else {
		Logger::warning("Unknown hdmi write: %s", data.hexstring());
	}
}

bool HDMIController::check_interrupts() {
	return interrupt_info[0];
}


I2CController::I2CController(Hardware *hardware, I2CDevice *device, bool espresso) {
	this->hardware = hardware;
	this->espresso = espresso;
	this->device = device;
}

void I2CController::reset() {
	clock = 0;
	writeval = 0;
	
	int_mask = 0;
	int_state = 0;
	
	writebuf.clear();
	readbuf.clear();
	
	device->reset();
}

uint32_t I2CController::read(uint32_t addr) {
	switch (addr) {
		case I2C_CLOCK: return clock;
		case I2C_WRITE_DATA: return writeval;
		case I2C_READ_DATA:
			if (readoffs == -1) {
				readoffs++;
				return readbuf.size() << 16;
			}
			return readbuf.at(readoffs++);
		case I2C_WRITE_CTRL: return 0;
		case I2C_INT_MASK: return int_mask;
		case I2C_INT_STATE: return int_state;
	}
	
	Logger::warning("Unknown i2c read: 0x%X", addr);
	return 0;
}

void I2CController::write(uint32_t addr, uint32_t value) {
	if (addr == I2C_CLOCK) clock = value;
	else if (addr == I2C_WRITE_DATA) writeval = value;
	else if (addr == I2C_WRITE_CTRL) {
		if (value & 1) {
			writebuf.push_back(writeval & 0xFF);
			if (writeval & 0x100) {
				process_write();
				writebuf.clear();
			}
		}
	}
	else if (addr == I2C_INT_MASK) {
		int_mask = value;
	}
	else if (addr == I2C_INT_STATE) int_state &= ~value;
	else {
		Logger::warning("Unknown i2c write: 0x%X (0x%08X)", addr, value);
	}
}

void I2CController::process_write() {
	Buffer data(&writebuf[0], writebuf.size(), Buffer::CreateCopy);
	
	bool read = writebuf[0] & 1;
	int slave = writebuf[0] >> 1;
	
	Buffer payload = data.slice(1, data.size());
	
	if (read) {
		readoffs = espresso ? 0 : -1;
		
		readbuf.resize(payload.size());
		
		device->read(slave, &readbuf[0], readbuf.size());

		trigger_interrupt(espresso ? 5 : 0);
	}
	else {
		device->write(slave, payload);
		trigger_interrupt(espresso ? 6 : 1);
	}
}

void I2CController::trigger_interrupt(int type) {
	int_state |= 1 << type;
}

void I2CController::update() {
	if (int_state & int_mask) {
		if (espresso) {
			hardware->trigger_irq_lt(Hardware::PPC, 13);
		}
		else {
			hardware->trigger_irq_lt(Hardware::ARM, 14);
		}
	}
}
