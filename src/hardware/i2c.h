
#pragma once

#include "common/buffer.h"

#include <vector>

#include <cstdint>


class Hardware;


class I2CDevice {
public:
	virtual void reset() = 0;
	
	virtual void write(int slave, Buffer data) = 0;
	virtual void read(int slave, uint8_t *data, size_t size) = 0;
};


class SMCController : public I2CDevice {
public:
	void reset();
	
	void write(int slave, Buffer data);
	void read(int slave, uint8_t *data, size_t size);
	
private:
	uint8_t system_event_flag;
	uint8_t usb_control;
	uint8_t power_failure_state;
	uint8_t wifi_reset;
	uint8_t data;
};


class HDMIController : public I2CDevice {
public:
	void reset();
	
	void write(int slave, Buffer data);
	void read(int slave, uint8_t *data, size_t size);
	
	bool check_interrupts();
	
private:
	uint8_t data;
	
	uint8_t interrupt_info[7];
};


class I2CController {
public:
	enum Register {
		I2C_CLOCK = 0,
		I2C_WRITE_DATA = 4,
		I2C_WRITE_CTRL = 8,
		I2C_READ_DATA = 0xC,
		I2C_INT_MASK = 0x10,
		I2C_INT_STATE = 0x14
	};
	
	I2CController(Hardware *hardware, I2CDevice *device, bool espresso);
	
	void reset();
	void update();
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
private:
	void process_write();
	void trigger_interrupt(int type);
	
	Hardware *hardware;
	I2CDevice *device;
	bool espresso;

	uint32_t clock;
	uint32_t writeval;
	
	uint32_t int_mask;
	uint32_t int_state;
	
	std::vector<uint8_t> writebuf;
	std::vector<uint8_t> readbuf;
	uint32_t readoffs;
};
