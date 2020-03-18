
#include "hardware/sdio.h"
#include "common/logger.h"
#include "physicalmemory.h"
#include "hardware.h"


SDIOCard::~SDIOCard() {}

MLCCard::MLCCard() {
	int fd = open("files/mlc.bin", O_RDWR);
	if (fd < 0) {
		runtime_error("Failed to open mlc.bin");
	}
	
	data = (uint8_t *)mmap(
		NULL, 0x76E000000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0
	);
	
	close(fd);
}

MLCCard::~MLCCard() {
	munmap(data, 0x76E000000);
}

Buffer MLCCard::read(uint64_t offset, uint32_t size) {
	return Buffer(data + offset, size, Buffer::CreateCopy);
}


Buffer DummyCard::read(uint64_t offset, uint32_t size) {
	Logger::warning("Unknown sdio controller read");
	return Buffer(size);
}


uint8_t SDIOCardInfo[] = {
	0x22, 0x04, 0x00, 0xFF, 0xFF, 0x32, 0xFF, 0x00
};

uint32_t SDIOCardData[] = {
	0x400E0032, 0x5B590000, 0xFFFF7F80, 0x0A400001
};

SDIOController::SDIOController(Hardware *hardware, PhysicalMemory *physmem, Type type) {
	this->hardware = hardware;
	this->physmem = physmem;
	this->type = type;
	
	if (type == TYPE_MLC) card = new MLCCard();
	else {
		card = new DummyCard();
	}
}

SDIOController::~SDIOController() {
	delete card;
}

void SDIOController::reset() {
	argument = 0;
	transfer_mode = 0;
	command = 0;
	result0 = 0;
	result1 = 0;
	result2 = 0;
	result3 = 0;
	control = 0;
	clock_control = 0;
	timeout_control = 0;
	int_status = 0;
	int_enable = 0;
	int_signal = 0;
	capabilities = 0;
	
	state = STATE_IDLE;
	app_cmd = false;
	
	bus_width = 0;
	cd_disable = 0;
}

uint32_t SDIOController::read(uint32_t addr) {
	switch (addr) {
		case SDIO_COMMAND: return (command << 16) | transfer_mode;
		case SDIO_RESPONSE0: return result0;
		case SDIO_RESPONSE1: return result1;
		case SDIO_RESPONSE2: return result2;
		case SDIO_RESPONSE3: return result3;
		case SDIO_STATE: return 0;
		case SDIO_CONTROL: return control;
		case SDIO_CLOCK_CONTROL: return clock_control | (timeout_control << 16);
		case SDIO_INT_STATUS: return int_status;
		case SDIO_CAPABILITIES_HI: return capabilities >> 32;
	}
	
	Logger::warning("Unknown sdio read: 0x%X", addr);
	return 0;
}

void SDIOController::write(uint32_t addr, uint32_t value) {
	if (addr == SDIO_DMA_ADDRESS) dma_addr = value;
	else if (addr == SDIO_BLOCK_SIZE) {
		block_size = value & 0xFFF;
		block_count = value >> 16;
	}
	else if (addr == SDIO_ARGUMENT) argument = value;
	else if (addr == SDIO_COMMAND) {
		transfer_mode = value & 0xFFFF;
		command = value >> 16;
		
		result0 = 0;
		result1 = 0;
		result2 = 0;
		result3 = 0;
		
		int index = (command >> 8) & 0x3F;
		if (app_cmd) {
			process_app_command(index);
			app_cmd = false;
		}
		else {
			process_command(index);
		}
		
		int_status |= 3;
	}
	else if (addr == SDIO_CONTROL) control = value;
	else if (addr == SDIO_CLOCK_CONTROL) {
		clock_control = value & 0xFFFF;
		if (clock_control & 1) {
			clock_control |= 2;
		}
		timeout_control = (value >> 16) & 0xFF;
		
		int software_reset = (value >> 24) & 0xFF;
		if (software_reset & 1) {
			reset();
		}
	}
	else if (addr == SDIO_INT_STATUS) int_status &= value ^ 0xFFFFFF;
	else if (addr == SDIO_INT_ENABLE) {
		int_enable = value;
	}
	else if (addr == SDIO_INT_SIGNAL) {
		int_signal = value;
	}
	else {
		Logger::warning("Unknown sdio write: 0x%X (0x%08X)", addr, value);
	}
}

void SDIOController::process_app_command(int command) {
	if (command == SET_BUS_WIDTH) {
		bus_width = argument & 3;
		result0 = (state << 9) | 0x120;
	}
	else if (command == SD_STATUS) result0 = (state << 9) | 0x120;
	else if (command == SD_SEND_OP_COND) {
		result0 = 0x300000;
		if (argument & result0) {
			result0 |= 0xC0000000;
		}
	}
	else {
		Logger::warning("Unknown sdio app command: %i", command);
	}
}

void SDIOController::process_command(int command) {
	if (command == GO_IDLE_STATE) {
		state = STATE_IDLE;
	}
	else if (command == ALL_SEND_CID) {}
	else if (command == SEND_RELATIVE_ADDR) result0 = 0x400;
	else if (command == IO_SEND_OP_COND) {
		int_status |= 0x8000;
	}
	else if (command == SWITCH_FUNC) {}
	else if (command == SELECT_CARD) {
		state = STATE_STBY;
		result0 = (state << 9) | 0x100;
	}
	else if (command == SEND_IF_COND) result0 = argument & 0xFFF;
	else if (command == SEND_CSD) {
		result3 = (SDIOCardData[0] >> 8) | (SDIOCardData[3] << 24);
		result2 = (SDIOCardData[1] >> 8) | (SDIOCardData[0] << 24);
		result1 = (SDIOCardData[2] >> 8) | (SDIOCardData[1] << 24);
		result0 = (SDIOCardData[3] >> 8) | (SDIOCardData[2] << 24);
	}
	else if (command == SEND_STATUS) {
		state = STATE_TRAN;
		result0 = (state << 9) | 0x100;
	}
	else if (command == SET_BLOCKLEN) {
		result0 = (state << 9) | 0x100;
	}
	else if (command == READ_MULTIPLE_BLOCK) {
		Buffer data = card->read((uint64_t)argument << 9, block_count * block_size);
		physmem->write(dma_addr, data);
	}
	else if (command == IO_RW_DIRECT) {
		int function = (argument >> 28) & 7;
		int address = (argument >> 9) & 0x1FFFF;
		if (argument >> 31) {
			write_register(function, address, argument & 0xFF);
			if ((argument >> 27) & 1) {
				result0 = read_register(function, address);
			}
		}
		else {
			result0 = read_register(function, address);
		}
	}
	else if (command == APP_CMD) {
		result0 = 0x20;
		app_cmd = true;
	}
	else {
		Logger::warning("Unknown sdio command: %i", command);
	}
}

uint8_t SDIOController::read_register(int function, int address) {
	if (function == 0) {
		switch (address) {
			case 7: return bus_width | (cd_disable << 7);
			case 9: return 0;
			case 10: return 0x10;
			case 11: return 0;
			case 19: return 1;
		}
		
		if (address >= 0x1000 && address < 0x1000 + sizeof(SDIOCardInfo)) {
			return SDIOCardInfo[address - 0x1000];
		}
	}
	
	Logger::warning("Unknown sdio register read: %i %i", function, address);
	return 0;
}

void SDIOController::write_register(int function, int address, uint8_t value) {
	if (function == 0) {
		if (address == 6) {}
		else if (address == 7) {
			bus_width = value & 3;
			cd_disable = value >> 7;
		}
		else {
			Logger::warning("Unknown sdio register write: %i %i (%i)", function, address, value);
		}
	}
	else {
		Logger::warning("Unknown sdio register write: %i %i (%i)", function, address, value);
	}
}

void SDIOController::update() {
	if (int_status & int_enable & int_signal) {
		if (type == TYPE_SD) hardware->trigger_irq_all(Hardware::ARM, 6);
		else if (type == TYPE_WIFI) hardware->trigger_irq_all(Hardware::ARM, 7);
		else if (type == TYPE_MLC) hardware->trigger_irq_lt(Hardware::ARM, 0);
		else if (type == TYPE_UNK) hardware->trigger_irq_lt(Hardware::ARM, 1);
	}
}
