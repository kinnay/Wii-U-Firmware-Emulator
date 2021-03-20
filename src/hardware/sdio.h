
#pragma once

#include "common/buffer.h"


class PhysicalMemory;


class SDIOCard {
public:
	virtual ~SDIOCard();
	
	virtual Buffer read(uint64_t offset, uint32_t size) = 0;
};


class MLCCard : public SDIOCard {
public:
	MLCCard();
	~MLCCard();
	
	Buffer read(uint64_t offset, uint32_t size);
	
private:
	uint8_t *data;
};


class DummyCard : public SDIOCard {
public:
	Buffer read(uint64_t offset, uint32_t size);
};


class SDIOController {
public:
	enum Type {
		TYPE_SD, TYPE_MLC, TYPE_UNK, TYPE_WIFI
	};
	
	enum Register {
		SDIO_DMA_ADDRESS = 0,
		SDIO_BLOCK_SIZE = 4,
		SDIO_ARGUMENT = 8,
		SDIO_COMMAND = 0xC,
		SDIO_RESPONSE0 = 0x10,
		SDIO_RESPONSE1 = 0x14,
		SDIO_RESPONSE2 = 0x18,
		SDIO_RESPONSE3 = 0x1C,
		SDIO_STATE = 0x24,
		SDIO_CONTROL = 0x28,
		SDIO_CLOCK_CONTROL = 0x2C,
		SDIO_INT_STATUS = 0x30,
		SDIO_INT_ENABLE = 0x34,
		SDIO_INT_SIGNAL = 0x38,
		SDIO_CAPABILITIES_HI = 0x40
	};
	
	enum Command {
		GO_IDLE_STATE = 0,
		ALL_SEND_CID = 2,
		SEND_RELATIVE_ADDR = 3,
		IO_SEND_OP_COND = 5,
		SWITCH_FUNC = 6,
		SELECT_CARD = 7,
		SEND_IF_COND = 8,
		SEND_CSD = 9,
		SEND_STATUS = 13,
		SET_BLOCKLEN = 16,
		READ_MULTIPLE_BLOCK = 18,
		IO_RW_DIRECT = 52,
		APP_CMD = 55
	};
	
	enum AppCommand {
		SET_BUS_WIDTH = 6,
		SD_STATUS = 13,
		SD_SEND_OP_COND = 41
	};
	
	enum State {
		STATE_IDLE = 0,
		STATE_READY = 1,
		STATE_IDENT = 2,
		STATE_STBY = 3,
		STATE_TRAN = 4,
		STATE_DATA = 5,
		STATE_RCV = 6,
		STATE_PRG = 7,
		STATE_DIS = 8
	};
	
	SDIOController(PhysicalMemory *physmem, Type type);
	~SDIOController();
	
	void reset();
	void update();
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
	bool check_interrupts();
	
private:
	void process_app_command(int command);
	void process_command(int command);
	
	uint8_t read_register(int function, int address);
	void write_register(int function, int address, uint8_t value);
	
	PhysicalMemory *physmem;
	SDIOCard *card;
	Type type;
	
	State state;
	bool app_cmd;
	
	uint32_t dma_addr;
	uint16_t block_size;
	uint16_t block_count;
	uint32_t argument;
	uint16_t transfer_mode;
	uint16_t command;
	uint32_t result0;
	uint32_t result1;
	uint32_t result2;
	uint32_t result3;
	uint32_t control;
	uint16_t clock_control;
	uint8_t timeout_control;
	uint32_t int_status;
	uint32_t int_enable;
	uint32_t int_signal;
	uint64_t capabilities;
	
	int bus_width;
	int cd_disable;
};
