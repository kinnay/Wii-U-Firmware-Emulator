
#include "x86codegenerator.h"

#include <cstdlib>


X86CodeGenerator::X86CodeGenerator(size_t size) {
	buffer = (char *)malloc(size);
	capacity = size;
	offset = 0;
}

X86CodeGenerator::~X86CodeGenerator() {
	free(buffer);
}

char *X86CodeGenerator::get() {
	return buffer;
}

void X86CodeGenerator::seek(size_t pos) {
	offset = pos;
}

size_t X86CodeGenerator::tell() {
	return offset;
}

size_t X86CodeGenerator::size() {
	return capacity;
}

void X86CodeGenerator::reserve(size_t size) {
	if (offset + size > capacity) {
		capacity *= 2;
		buffer = (char *)realloc(buffer, capacity);
	}
}

void X86CodeGenerator::u8(uint8_t value) {
	reserve(1);
	buffer[offset++] = value;
}

void X86CodeGenerator::u32(uint32_t value) {
	reserve(4);
	*(uint32_t *)(buffer + offset) = value;
	offset += 4;
}

void X86CodeGenerator::u64(uint64_t value) {
	reserve(8);
	*(uint64_t *)(buffer + offset) = value;
	offset += 8;
}

bool X86CodeGenerator::isU8(uint32_t value) {
	return value <= 0x7F || value >= 0xFFFFFF80;
}

void X86CodeGenerator::displace(uint8_t mod, Register base, uint32_t offset) {
	u8(mod | 0x80 | base);
	if (base == RSP) {
		u8(0x24);
	}
	u32(offset);
}

void X86CodeGenerator::rex() {
	u8(0x48);
}

void X86CodeGenerator::ret() {
	u8(0xC3);
}

void X86CodeGenerator::flipCarry() {
	u8(0xF5);
}

void X86CodeGenerator::pushReg64(Register reg) {
	u8(0x50 + reg);
}

void X86CodeGenerator::popReg64(Register reg) {
	u8(0x58 + reg);
}

void X86CodeGenerator::movReg32(Register dest, Register source) {
	u8(0x89);
	u8(0xC0 | (source << 3) | dest);
}

void X86CodeGenerator::movReg64(Register dest, Register source) {
	rex();
	u8(0x89);
	u8(0xC0 | (source << 3) | dest);
}

void X86CodeGenerator::movImm32(Register dest, uint32_t value) {
	u8(0xB8 + dest);
	u32(value);
}

void X86CodeGenerator::movImm64(Register dest, uint64_t value) {
	rex();
	u8(0xB8 + dest);
	u64(value);
}

void X86CodeGenerator::loadMem32(Register dest, Register base, uint32_t offset) {
	u8(0x8B);
	displace(dest << 3, base, offset);
}

void X86CodeGenerator::loadMem64(Register dest, Register base, uint32_t offset) {
	rex();
	u8(0x8B);
	displace(dest << 3, base, offset);
}

void X86CodeGenerator::storeMem32(Register base, uint32_t offset, Register source) {
	u8(0x89);
	displace(source << 3, base, offset);
}

void X86CodeGenerator::storeMemImm32(Register base, uint32_t offset, uint32_t value) {
	u8(0xC7);
	displace(0, base, offset);
	u32(value);
}

void X86CodeGenerator::lea64(Register reg, Register base, uint32_t offset) {
	rex();
	u8(0x8D);
	displace(reg << 3, base, offset);
}

void X86CodeGenerator::swap32(Register reg) {
	u8(0x0F);
	u8(0xC8 + reg);
}

void X86CodeGenerator::addRegReg32(Register reg, Register other) {
	u8(0x01);
	u8(0xC0 | (other << 3) | reg);
}

void X86CodeGenerator::addRegImm32(Register reg, uint32_t value) {
	u8(0x81);
	u8(0xC0 | reg);
	u32(value);
}

void X86CodeGenerator::addRegImm64(Register reg, uint32_t value) {
	rex();
	addRegImm32(reg, value);
}

void X86CodeGenerator::addMemReg32(Register base, uint32_t offset, Register other) {
	u8(0x01);
	displace(other << 3, base, offset);
}

void X86CodeGenerator::addMemImm32(Register base, uint32_t offset, uint32_t value) {
	u8(0x81);
	displace(0, base, offset);
	u32(value);
}

void X86CodeGenerator::addRegMem32(Register reg, Register base, uint32_t offset) {
	u8(0x03);
	displace(reg << 3, base, offset);
}

void X86CodeGenerator::subRegReg32(Register reg, Register other) {
	u8(0x29);
	u8(0xC0 | (other << 3) | reg);
}

void X86CodeGenerator::subRegImm32(Register reg, uint32_t value) {
	u8(0x81);
	u8(0xE8 | reg);
	u32(value);
}

void X86CodeGenerator::subRegImm64(Register reg, uint32_t value) {
	rex();
	subRegImm32(reg, value);
}

void X86CodeGenerator::adcRegReg32(Register reg, Register other) {
	u8(0x11);
	u8(0xC0 | (other << 3) | reg);
}

void X86CodeGenerator::adcRegImm32(Register reg, uint32_t value) {
	u8(0x81);
	u8(0xD0 | reg);
	u32(value);
}

void X86CodeGenerator::sbbRegReg32(Register reg, Register other) {
	u8(0x19);
	u8(0xC0 | (other << 3) | reg);
}

void X86CodeGenerator::sbbRegImm32(Register reg, uint32_t value) {
	u8(0x81);
	u8(0xD8 | reg);
	u32(value);
}

void X86CodeGenerator::mulReg32(Register reg) {
	u8(0xF7);
	u8(0xE0 | reg);
}

void X86CodeGenerator::negReg32(Register reg) {
	u8(0xF7);
	u8(0xD8 | reg);
}

void X86CodeGenerator::decMem32(Register base, uint32_t offset) {
	u8(0xFF);
	displace(8, base, offset);
}

void X86CodeGenerator::shlReg32(Register reg) {
	u8(0xD3);
	u8(0xE0 | reg);
}

void X86CodeGenerator::shrReg32(Register reg) {
	u8(0xD3);
	u8(0xE8 | reg);
}

void X86CodeGenerator::sarReg32(Register reg) {
	u8(0xD3);
	u8(0xF8 | reg);
}

void X86CodeGenerator::rolReg32(Register reg) {
	u8(0xD3);
	u8(0xC0 | reg);
}

void X86CodeGenerator::rorReg32(Register reg) {
	u8(0xD3);
	u8(0xC8 | reg);
}

void X86CodeGenerator::shlImm32(Register reg, uint8_t bits) {
	if (bits == 1) {
		u8(0xD1);
		u8(0xE0 | reg);
	}
	else {
		u8(0xC1);
		u8(0xE0 | reg);
		u8(bits);
	}
}

void X86CodeGenerator::shrImm32(Register reg, uint8_t bits) {
	if (bits == 1) {
		u8(0xD1);
		u8(0xE8 | reg);
	}
	else {
		u8(0xC1);
		u8(0xE8 | reg);
		u8(bits);
	}
}

void X86CodeGenerator::sarImm32(Register reg, uint8_t bits) {
	if (bits == 1) {
		u8(0xD1);
		u8(0xF8 | reg);
	}
	else {
		u8(0xC1);
		u8(0xF8 | reg);
		u8(bits);
	}
}

void X86CodeGenerator::rolImm32(Register reg, uint8_t bits) {
	if (bits == 1) {
		u8(0xD1);
		u8(0xC0 | reg);
	}
	else {
		u8(0xC1);
		u8(0xC0 | reg);
		u8(bits);
	}
}

void X86CodeGenerator::rorImm32(Register reg, uint8_t bits) {
	if (bits == 1) {
		u8(0xD1);
		u8(0xC8 | reg);
	}
	else {
		u8(0xC1);
		u8(0xC8 | reg);
		u8(bits);
	}
}

void X86CodeGenerator::rcrImm32(Register reg, uint8_t bits) {
	if (bits == 1) {
		u8(0xD1);
		u8(0xD8 | reg);
	}
	else {
		u8(0xC1);
		u8(0xD8 | reg);
		u8(bits);
	}
}

void X86CodeGenerator::andReg32(Register reg, Register other) {
	u8(0x21);
	u8(0xC0 | (other << 3) | reg);
}

void X86CodeGenerator::andImm32(Register reg, uint32_t value) {
	u8(0x81);
	u8(0xE0 | reg);
	u32(value);
}

void X86CodeGenerator::andMemImm32(Register base, uint32_t offset, uint32_t value) {
	u8(0x81);
	displace(0x20, base, offset);
	u32(value);
}

void X86CodeGenerator::orImm32(Register reg, uint32_t value) {
	u8(0x81);
	u8(0xC8 | reg);
	u32(value);
}

void X86CodeGenerator::orReg32(Register reg, Register other) {
	u8(0x0B);
	u8(0xC0 | (reg << 3) | other);
}

void X86CodeGenerator::orMemReg32(Register base, uint32_t offset, Register reg) {
	u8(0x09);
	displace(reg << 3, base, offset);
}

void X86CodeGenerator::orMemImm32(Register base, uint32_t offset, uint32_t value) {
	u8(0x81);
	displace(8, base, offset);
	u32(value);
}

void X86CodeGenerator::xorImm32(Register reg, uint32_t value) {
	u8(0x81);
	u8(0xF0 | reg);
	u32(value);
}

void X86CodeGenerator::xorReg32(Register reg, Register other) {
	u8(0x33);
	u8(0xC0 | (reg << 3) | other);
}

void X86CodeGenerator::notReg32(Register reg) {
	u8(0xF7);
	u8(0xD0 | reg);
}

void X86CodeGenerator::callAbs(Register temp, uint64_t addr) {
	movImm64(temp, addr);
	u8(0xFF);
	u8(0xD0 | temp);
}

void X86CodeGenerator::jumpRel(uint32_t offset) {
	if (isU8(offset)) {
		u8(0xEB);
		u8(offset);
	}
	else {
		u8(0xE9);
		u32(offset);
	}
}

void X86CodeGenerator::jumpAbs(Register temp, uint64_t addr) {
	movImm64(temp, addr);
	u8(0xFF);
	u8(0xE0 | temp);
}

void X86CodeGenerator::jumpIfCarry(uint8_t offset) {
	u8(0x72);
	u8(offset);
}

void X86CodeGenerator::jumpIfNotCarry(uint8_t offset) {
	u8(0x73);
	u8(offset);
}

void X86CodeGenerator::jumpIfOverflow(uint8_t offset) {
	u8(0x70);
	u8(offset);
}

void X86CodeGenerator::jumpIfNotOverflow(uint8_t offset) {
	u8(0x71);
	u8(offset);
}

void X86CodeGenerator::jumpIfZero(uint8_t offset) {
	u8(0x74);
	u8(offset);
}

void X86CodeGenerator::jumpIfNotZero(uint8_t offset) {
	u8(0x75);
	u8(offset);
}

void X86CodeGenerator::jumpIfSign(uint8_t offset) {
	u8(0x78);
	u8(offset);
}

void X86CodeGenerator::jumpIfNotSign(uint8_t offset) {
	u8(0x79);
	u8(offset);
}

void X86CodeGenerator::jumpIfEqual(uint8_t offset) {
	u8(0x74);
	u8(offset);
}

void X86CodeGenerator::jumpIfNotEqual(uint8_t offset) {
	u8(0x75);
	u8(offset);
}

void X86CodeGenerator::jumpIfBelow(uint8_t offset) {
	u8(0x72);
	u8(offset);
}

void X86CodeGenerator::compareImm32(Register reg, uint32_t value) {
	if (reg == RAX) {
		u8(0x3D);
		u32(value);
	}
	else {
		u8(0x81);
		u8(0xF8 | reg);
		u32(value);
	}
}

void X86CodeGenerator::testReg32(Register a, Register b) {
	u8(0x85);
	u8(0xC0 | (b << 3) | a);
}

void X86CodeGenerator::bitTestReg32(Register reg, uint8_t bit) {
	u8(0x0F);
	u8(0xBA);
	u8(0xE0 | reg);
	u8(bit);
}

void X86CodeGenerator::bitTestMem32(Register base, uint32_t offset, uint8_t bit) {
	u8(0x0F);
	u8(0xBA);
	displace(0x20, base, offset);
	u8(bit);
}

void X86CodeGenerator::bitTestSetMem32(Register base, uint32_t offset, uint8_t bit) {
	u8(0x0F);
	u8(0xBA);
	displace(0x28, base, offset);
	u8(bit);
}

void X86CodeGenerator::bitTestResetMem32(Register base, uint32_t offset, uint8_t bit) {
	u8(0x0F);
	u8(0xBA);
	displace(0x30, base, offset);
	u8(bit);
}

void X86CodeGenerator::bitTestResetReg32(Register reg, uint8_t bit) {
	u8(0x0F);
	u8(0xBA);
	u8(0xF0 | reg);
	u8(bit);
}
