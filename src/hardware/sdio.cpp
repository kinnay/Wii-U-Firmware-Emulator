
#include "hardware/sdio.h"
#include "common/logger.h"
#include "common/memorymappedfile.h"
#include "physicalmemory.h"
#include "hardware.h"

#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

SDIOCard::SDIOCard() : csd() {
	csd.csd_structure = 1; // version 2
	csd.taac = 0xE; // 1 ms
	csd.nsac = 0;
	csd.tran_speed = 0x32; // 25 Mbit/s
	csd.ccc = 0x5B5;
	csd.read_bl_len = 0x9; // 512 bytes
	csd.read_bl_partial = 0;
	csd.write_blk_misalign = 0;
	csd.read_blk_misalign = 0;
	csd.dsr_imp = 0;
	csd.csize_hi = 0;
	csd.csize_lo = 0;
	csd.erase_blk_enable = 0x1;
	csd.sector_size = 0x7F; // 128 blocks
	csd.wp_grp_size = 0;
	csd.wp_grp_enable = 0;
	csd.r2w_factor = 0x2; // x4
	csd.write_bl_len = 0x9; // 512 bytes
	csd.write_grp_enable = 0;
	csd.file_format_grp = 0;
	csd.copy = 0;
	csd.perm_write_protect = 0;
	csd.twp_write_protect = 0;
	csd.file_format = 0;
	csd.crc = 0x01;
}

SDIOCard::~SDIOCard() {}

MLCCard::MLCCard() {
	FILE* f = fopen64("files/mlc.bin", "rb");
	if (!f) {
		runtime_error("Failed to open \"files/mlc.bin\"");
	}
	fseek(f, 0, SEEK_END);
	loff_t size = ftello64(f);
	fclose(f);

	is_32gb = size >= 0x1DB800000;

	csd.csize_lo = is_32gb ? 0xFFFF : 0x3FFF;

	data = memory_mapped_file_open("files/mlc.bin", is_32gb ? 0x76E000000 : 0x1DB800000);
}

MLCCard::~MLCCard() {
	memory_mapped_file_close(data, is_32gb ? 0x76E000000 : 0x1DB800000);
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

SDIOController::SDIOController(PhysicalMemory *physmem, Type type) {
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
		result3 = (card->csd.data[0] >> 8) | (card->csd.data[3] << 24);
		result2 = (card->csd.data[1] >> 8) | (card->csd.data[0] << 24);
		result1 = (card->csd.data[2] >> 8) | (card->csd.data[1] << 24);
		result0 = (card->csd.data[3] >> 8) | (card->csd.data[2] << 24);
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

bool SDIOController::check_interrupts() {
	return int_status & int_enable & int_signal;
}
