
#pragma once

#include "common/buffer.h"
#include "hardware/i2c.h"

#include <cstdint>


class Hardware;


class SEEPROMController {
public:
	enum State {
		LISTEN,
		READ,
		WRITE,
		WRITE_DELAY,
		DELAY
	};
	
	void reset();
	void update();
	void write(bool value);
	bool read();
	
private:
	void prepare();
	void handle_command();
	void handle_write();
	
	State state;
	int cycles;
	int offset;
	uint16_t value;
	bool pin;
	
	Buffer buffer;
	uint16_t *data;
};


class GPIOGroup {
public:
	virtual void reset() = 0;
	virtual uint32_t read() = 0;
	virtual void write(int pin, bool state) = 0;
};


class GPIOCommon : public GPIOGroup {
public:
	enum Pin {
		PIN_EEPROM_CS = 10,
		PIN_EEPROM_SK = 11,
		PIN_EEPROM_DO = 12,
		PIN_EEPROM_DI = 13,
		PIN_DEBUG0 = 16,
		PIN_DEBUG1 = 17,
		PIN_DEBUG2 = 18,
		PIN_DEBUG3 = 19,
		PIN_DEBUG4 = 20,
		PIN_DEBUG5 = 21,
		PIN_DEBUG6 = 22,
		PIN_DEBUG7 = 23,
		PIN_TOUCAN = 31
	};
	
	void reset();
	
	uint32_t read();
	void write(int pin, bool state);
	
private:
	SEEPROMController seeprom;
};


class GPIOLatte : public GPIOGroup {
public:
	enum Pin {
	};
	
	GPIOLatte(HDMIController *hdmi);
	
	void reset();
	
	uint32_t read();
	void write(int pin, bool state);
	
private:
	HDMIController *hdmi;
};


class GPIOController {
public:
	enum Register {
		LT_GPIOE_OUT = 0,
		LT_GPIOE_DIR = 4,
		LT_GPIOE_IN = 8,
		LT_GPIOE_INTLVL = 0xC,
		LT_GPIOE_INTFLAG = 0x10,
		LT_GPIOE_INTMASK = 0x14,
		LT_GPIOE_INMIR = 0x18,
		
		LT_GPIO_ENABLE = 0x1C,
		LT_GPIO_OUT = 0x20,
		LT_GPIO_DIR = 0x24,
		LT_GPIO_IN = 0x28,
		LT_GPIO_INTLVL = 0x2C,
		LT_GPIO_INTFLAG = 0x30,
		LT_GPIO_INTMASK = 0x34,
		LT_GPIO_INMIR = 0x38,
		LT_GPIO_OWNER = 0x3C
	};
	
	GPIOController(GPIOGroup *group);
	
	void reset();
	void update();
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
	bool check_interrupts(bool ppc);
	
private:
	uint32_t gpio_enable;
	uint32_t gpio_out;
	uint32_t gpio_dir;
	uint32_t gpio_intlvl;
	uint32_t gpio_intflag;
	uint32_t gpio_intmask;
	uint32_t gpio_owner;
	
	GPIOGroup *group;
};
