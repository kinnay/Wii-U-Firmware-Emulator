
#include "common/logger.h"
#include "common/fileutils.h"
#include "common/endian.h"
#include "hardware/gpio.h"
#include "hardware.h"


void SEEPROMController::reset() {
	buffer = FileUtils::load("files/seeprom.bin");
	data = (uint16_t *)buffer.get();
	
	state = LISTEN;
	
	cycles = 11;
	offset = 0;
	value = 0;
	pin = false;
}

void SEEPROMController::prepare() {
	state = LISTEN;
	cycles = 11;
	value = 0;
}

void SEEPROMController::write(bool state) {
	pin = state;
}

bool SEEPROMController::read() {
	return pin;
}

void SEEPROMController::update() {
	if (state == LISTEN) {
		value = (value << 1) | pin;
		if (--cycles == 0) {
			handle_command();
		}
	}
	else if (state == WRITE) {
		value = (value << 1) | pin;
		if (--cycles == 0) {
			handle_write();
		}
	}
	else if (state == WRITE_DELAY) {
		if (--cycles == 0) {
			value = 1;
			cycles = 1;
			state = READ;
		}
	}
	else if (state == READ) {
		pin = (value >> --cycles) & 1;
		if (cycles == 0) {
			cycles = 2;
			state = DELAY;
		}
	}
	else if (state == DELAY) {
		if (--cycles == 0) {
			prepare();
		}
	}
}

void SEEPROMController::handle_command() {
	int command = value >> 8;
	int param = value & 0xFF;
	
	if (command == 4) {
		cycles = 2;
		state = DELAY;
	}
	else if (command == 5) {
		cycles = 16;
		value = 0;
		offset = param;
		state = WRITE;
	}
	else if (command == 6) {
		cycles = 16;
		value = Endian::swap16(data[param]);
		state = READ;
	}
	else {
		Logger::warning("Unknown seeprom command: 0x%X", value);
	}
}

void SEEPROMController::handle_write() {
	data[offset] = Endian::swap16(value);
	state = WRITE_DELAY;
	cycles = 2;
}


void GPIOCommon::reset() {
	seeprom.reset();
}

uint32_t GPIOCommon::read() {
	return seeprom.read() << PIN_EEPROM_DI;
}

void GPIOCommon::write(int pin, bool state) {
	if (pin == PIN_EEPROM_CS) {}
	else if (pin == PIN_EEPROM_SK) {
		if (state) seeprom.update();
	}
	else if (pin == PIN_EEPROM_DO) seeprom.write(state);
	else if (pin == PIN_DEBUG0) Logger::info("Debug 0: %b", state);
	else if (pin == PIN_DEBUG1) Logger::info("Debug 1: %b", state);
	else if (pin == PIN_DEBUG2) Logger::info("Debug 2: %b", state);
	else if (pin == PIN_DEBUG3) Logger::info("Debug 3: %b", state);
	else if (pin == PIN_DEBUG4) Logger::info("Debug 4: %b", state);
	else if (pin == PIN_DEBUG5) Logger::info("Debug 5: %b", state);
	else if (pin == PIN_DEBUG6) Logger::info("Debug 6: %b", state);
	else if (pin == PIN_DEBUG7) Logger::info("Debug 7: %b", state);
	else if (pin == PIN_TOUCAN) {}
	else {
		Logger::warning("Unknown common gpio pin write: %i (%b)", pin, state);
	}
}


GPIOLatte::GPIOLatte(HDMIController *hdmi) {
	this->hdmi = hdmi;
}

void GPIOLatte::reset() {}

uint32_t GPIOLatte::read() {
	return hdmi->check_interrupts() << 4;
}

void GPIOLatte::write(int pin, bool state) {
	Logger::warning("Unknown latte gpio pin write: %i (%b)", pin, state);
}


GPIOController::GPIOController(GPIOGroup *group) {
	this->group = group;
}

void GPIOController::reset() {
	gpio_enable = 0;
	gpio_out = 0;
	gpio_dir = 0;
	gpio_intlvl = 0;
	gpio_intflag = 0;
	gpio_intmask = 0;
	gpio_owner = 0;
	
	group->reset();
}

uint32_t GPIOController::read(uint32_t addr) {
	switch (addr) {
		case LT_GPIOE_OUT: return gpio_out & gpio_owner;
		case LT_GPIOE_DIR: return gpio_dir & gpio_owner;
		case LT_GPIOE_INTLVL: return gpio_intlvl & gpio_owner;
		case LT_GPIOE_INTFLAG: return gpio_intflag & gpio_owner;
		case LT_GPIOE_INTMASK: return gpio_intmask & gpio_owner;
		
		case LT_GPIO_ENABLE: return gpio_enable;
		case LT_GPIO_OUT: return gpio_out;
		case LT_GPIO_DIR: return gpio_dir;
		case LT_GPIO_IN: return group->read() & ~gpio_dir;
		case LT_GPIO_INTLVL: return gpio_intlvl;
		case LT_GPIO_INTFLAG: return gpio_intflag;
		case LT_GPIO_INTMASK: return gpio_intmask;
		case LT_GPIO_OWNER: return gpio_owner;
	}
	
	Logger::warning("Unknown gpio memory read: 0x%08X", addr);
	return 0;
}

void GPIOController::write(uint32_t addr, uint32_t value) {
	if (addr == LT_GPIOE_OUT) {
		uint32_t mask = gpio_dir & gpio_enable & gpio_owner;
		for (int i = 0; i < 32; i++) {
			if (mask & (1 << i)) {
				if ((value & (1 << i)) != (gpio_out & (1 << i))) {
					group->write(i, (value >> i) & 1);
				}
			}
		}
		gpio_out = (gpio_out & ~gpio_owner) | (value & gpio_owner);
	}
	else if (addr == LT_GPIOE_DIR) gpio_dir = (gpio_dir & ~gpio_owner) | (value & gpio_owner);
	else if (addr == LT_GPIOE_INTLVL) gpio_intlvl = (gpio_intlvl & ~gpio_owner) | (value & gpio_owner);
	else if (addr == LT_GPIOE_INTFLAG) gpio_intflag &= ~(value & gpio_owner);
	else if (addr == LT_GPIOE_INTMASK) gpio_intmask = (gpio_intmask & ~gpio_owner) | (value & gpio_owner);
	
	else if (addr == LT_GPIO_ENABLE) gpio_enable = value;
	else if (addr == LT_GPIO_OUT) {
		uint32_t mask = gpio_dir & gpio_enable & ~gpio_owner;
		for (int i = 0; i < 32; i++) {
			if (mask & (1 << i)) {
				if ((value & (1 << i)) != (gpio_out & (1 << i))) {
					group->write(i, (value >> i) & 1);
				}
			}
		}
		gpio_out = (gpio_out & gpio_owner) | (value & ~gpio_owner);
	}
	else if (addr == LT_GPIO_DIR) gpio_dir = (gpio_dir & gpio_owner) | (value & ~gpio_owner);
	else if (addr == LT_GPIO_INTLVL) gpio_intlvl = (gpio_intlvl & gpio_owner) | (value & ~gpio_owner);
	else if (addr == LT_GPIO_INTFLAG) gpio_intflag &= ~value;
	else if (addr == LT_GPIO_INTMASK) gpio_intmask = (gpio_intmask & gpio_owner) | (value & ~gpio_owner);
	else if (addr == LT_GPIO_OWNER) gpio_owner = value;
	else {
		Logger::warning("Unknown gpio memory write: 0x%08X", addr);
	}
}

void GPIOController::update() {
	gpio_intflag |= ~(gpio_intlvl ^ group->read());
}

bool GPIOController::check_interrupts(bool ppc) {
	uint32_t owner = ppc ? gpio_owner : ~gpio_owner;
	return gpio_intflag & gpio_intmask & owner;
}
