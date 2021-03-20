
#include "hardware/nand.h"

#include "common/logger.h"

#include "physicalmemory.h"

#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#include <cstring>


void NANDBank::prepare(PhysicalMemory *physmem, uint8_t *slc, uint8_t *slccmpt) {
	this->physmem = physmem;
	this->slc = slc;
	this->slccmpt = slccmpt;
}

void NANDBank::reset() {
	ctrl = 0;
	config = 0;
	addr1 = 0;
	addr2 = 0;
	databuf = 0;
	eccbuf = 0;
	
	pagenum = 0;
	pageoff = 0;
	
	file = slccmpt;
}

void NANDBank::set_bank(bool cmpt) {
	file = cmpt ? slccmpt : slc;
}

uint32_t NANDBank::read(uint32_t addr) {
	switch (addr) {
		case NAND_CTRL: return ctrl;
		case NAND_DATABUF: return databuf;
		case NAND_ECCBUF: return eccbuf;
	}
	
	Logger::warning("Unknown nand bank read: 0x%X", addr);
	return 0;
}

void NANDBank::write(uint32_t addr, uint32_t value) {
	if (addr == NAND_CTRL) {
		ctrl = value & ~0x80000000;
		if (value & 0x80000000) {
			process_ctrl(ctrl);
		}
	}
	else if (addr == NAND_CONFIG) config = value;
	else if (addr == NAND_ADDR1) addr1 = value;
	else if (addr == NAND_ADDR2) addr2 = value;
	else if (addr == NAND_DATABUF) databuf = value;
	else if (addr == NAND_ECCBUF) eccbuf = value;
	else {
		Logger::warning("Unknown nand bank write: 0x%X (0x%08X)", addr, value);
	}
}

void NANDBank::parse_addr(int flags) {
	if (flags & 1) pageoff = (pageoff & 0x700) | (addr1 & 0x0FF);
	if (flags & 2) pageoff = (pageoff & 0x0FF) | (addr1 & 0x700);
	if (flags & 4) pagenum = (pagenum & 0xFFFF00) | (addr2 & 0x0000FF);
	if (flags & 8) pagenum = (pagenum & 0xFF00FF) | (addr2 & 0x00FF00);
	if (flags & 16) pagenum = (pagenum & 0x00FFFF) | (addr2 & 0xFF0000);
}

void NANDBank::process_config() {
	process_ctrl(config);
	config &= ~0x80000000;
}

void NANDBank::process_ctrl(uint32_t value) {
	int addrmask = (value >> 24) & 0x1F;
	int command = (value >> 16) & 0xFF;
	int length = value & 0xFFF;
	
	parse_addr(addrmask);
	
	process_command(command, length);
	
	if (value & 0x40000000) {
		interrupt = true;
	}
}

void NANDBank::process_command(int command, int length) {
	if (command == 0x00) {} // Read (1st cycle)
	else if (command == 0x10) {} // Page program confirm
	else if (command == 0x30) { // Read (2nd cycle)
		uint32_t pagebase = pagenum * 0x840;
		if (length == 0x40) {
			physmem->write(databuf, file + pagebase + 0x800, 0x40);
		}
		else if (length == 0x840) {
			physmem->write(eccbuf, file + pagebase + 0x800, 0x40);
			physmem->write(eccbuf ^ 0x40, file + pagebase + 0x830, 0x10);
			
			physmem->write(databuf, file + pagebase + pageoff, 0x800 - pageoff);
			physmem->write(databuf + 0x800 - pageoff, file + pagebase + 0x840, pageoff);
		}
		else {
			Logger::warning("Read command has invalid size: 0x%X", length);
		}
	}
	else if (command == 0x60) {} // Block erase (1st cycle)
	else if (command == 0x70) { // Read status
		char data[0x40];
		memset(data, 0, sizeof(data));
		physmem->write(databuf, data, 0x40);
	}
	else if (command == 0x80) { // Page program
		uint32_t pagebase = pagenum * 0x840;
		physmem->read(databuf, file + pagebase + pageoff, 0x800 - pageoff);
		physmem->read(databuf + 0x800 - pageoff, file + pagebase + 0x840, pageoff);
	}
	else if (command == 0x85) { // Copy-back program
		uint32_t pagebase = pagenum * 0x840;
		physmem->read(databuf, file + pagebase + 0x800, 0x40);
	}
	else if (command == 0x90) { // Read ID
		physmem->write<uint16_t>(databuf, 0xECDC);
	}
	else if (command == 0xD0) {} // Block erase (2nd cycle)
	else if (command == 0xFF) { // Reset
		reset();
	}
	else {
		Logger::warning("Unknown nand command: 0x%02X", command);
	}
}

bool NANDBank::check_interrupts() {
	bool ints = interrupt;
	interrupt = false;
	return ints;
}


NANDController::NANDController(PhysicalMemory *physmem) {
	int fdslc = open("files/slc.bin", O_RDWR);
	if (fdslc < 0) {
		runtime_error("Failed to open slc.bin");
	}
	
	int fdcmpt = open("files/slccmpt.bin", O_RDWR);
	if (fdcmpt < 0) {
		runtime_error("Failed to open slccmpt.bin");
	}
	
	slc = (uint8_t *)mmap(
		NULL, 0x21000000, PROT_READ | PROT_WRITE, MAP_SHARED, fdslc, 0
	);
	slccmpt = (uint8_t *)mmap(
		NULL, 0x21000000, PROT_READ | PROT_WRITE, MAP_SHARED, fdcmpt, 0
	);
	
	close(fdslc);
	close(fdcmpt);
	
	main.prepare(physmem, slc, slccmpt);
	for (int i = 0; i < 8; i++) {
		banks[i].prepare(physmem, slc, slccmpt);
	}
}

NANDController::~NANDController() {
	munmap(slc, 0x21000000);
	munmap(slccmpt, 0x21000000);
}

void NANDController::reset() {
	bank_ctrl = 0;
	
	main.reset();
	for (int i = 0; i < 8; i++) {
		banks[i].reset();
	}
}

uint32_t NANDController::read(uint32_t addr) {
	switch (addr) {
		case NAND_BANK_CTRL: return bank_ctrl;
		case NAND_INT_MASK: return 0;
	}
	
	if (NAND_MAIN_START <= addr && addr < NAND_MAIN_END) return main.read(addr - NAND_MAIN_START);
	
	Logger::warning("Unknown nand read: 0x%X", addr);
	return 0;
}

void NANDController::write(uint32_t addr, uint32_t value) {
	if (addr == NAND_BANK) {
		bool cmpt = !(value & 2);
		main.set_bank(cmpt);
		for (int i = 0; i < 8; i++) {
			banks[i].set_bank(cmpt);
		}
	}
	else if (addr == NAND_BANK_CTRL) {
		bank_ctrl = value & ~0x80000000;
		if (value & 0x80000000) {
			int num = (value >> 16) & 0xFF;
			for (int i = 0; i < num; i++) {
				banks[i].process_config();
			}
		}
	}
	else if (NAND_MAIN_START <= addr && addr < NAND_MAIN_END) {
		main.write(addr - NAND_MAIN_START, value);
	}
	else if (NAND_BANKS_START <= addr && addr < NAND_BANKS_END) {
		addr -= NAND_BANKS_START;
		banks[addr / 0x18].write(addr % 0x18, value);
	}
	else {
		Logger::warning("Unknown nand write: 0x%X (0x%08X)", addr, value);
	}
}

bool NANDController::check_interrupts() {
	if (main.check_interrupts()) return true;
	for (int i = 0; i < 8; i++) {
		if (banks[i].check_interrupts()) return true;
	}
	return false;
}
