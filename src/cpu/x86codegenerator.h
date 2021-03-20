
#pragma once

#include <cstdint>
#include <cstddef>


enum Register {
	RAX, RCX, RDX, RBX, RSP, RBP, RSI, RDI
};


class X86CodeGenerator {
public:
	X86CodeGenerator(size_t size);
	~X86CodeGenerator();
	
	char *get();
	size_t size();
	
	void seek(size_t pos);
	size_t tell();
	
	void u8(uint8_t value); // 1 byte
	void u32(uint32_t value); // 4 bytes
	void u64(uint64_t value); // 8 bytes
	
	bool isU8(uint32_t value);
	void displace(uint8_t mod, Register base, uint32_t offset); // 2, 3, 5 or 6 bytes
	
	void rex(); // 1 byte
	
	void ret(); // 1 byte
	
	void flipCarry(); // 1 byte
	
	void pushReg64(Register reg); // 1 byte
	void popReg64(Register reg); // 1 byte
	
	void movReg32(Register dest, Register source); // 2 bytes
	void movReg64(Register dest, Register source); // 3 bytes
	void movImm32(Register dest, uint32_t value); // 5 bytes
	void movImm64(Register dest, uint64_t value); // 10 bytes
	
	void loadMem32(Register dest, Register base, uint32_t offset); // 6+ bytes
	void loadMem64(Register dest, Register base, uint32_t offset); // 7+ bytes
	
	void storeMem32(Register base, uint32_t offset, Register source); // 6+ bytes
	void storeMemImm32(Register base, uint32_t offset, uint32_t value); // 10+ bytes
	
	void lea64(Register reg, Register base, uint32_t offset); // 7+ bytes
	
	void swap32(Register reg); // 2 bytes
	
	void addRegReg32(Register reg, Register other); // 2 bytes
	void addRegImm32(Register reg, uint32_t value); // 6 bytes
	void addRegImm64(Register reg, uint32_t value); // 7 bytes
	void addMemReg32(Register base, uint32_t offset, Register other); // 6+ bytes
	void addMemImm32(Register base, uint32_t offset, uint32_t value); // 10+ bytes
	void addRegMem32(Register reg, Register base, uint32_t offset); // 6+ bytes
	
	void subRegReg32(Register reg, Register other); // 2 bytes
	void subRegImm32(Register reg, uint32_t value); // 6 bytes
	void subRegImm64(Register reg, uint32_t value); // 7 bytes
	
	void adcRegReg32(Register reg, Register other); // 2 bytes
	void adcRegImm32(Register reg, uint32_t imm); // 6 bytes
	
	void sbbRegReg32(Register reg, Register other); // 2 bytes
	void sbbRegImm32(Register reg, uint32_t imm); // 6 bytes
	
	void mulReg32(Register reg); // 2 bytes
	
	void negReg32(Register reg); // 2 bytes
	
	void decMem32(Register base, uint32_t offset); //6+ bytes
	
	void shlReg32(Register reg); // 2 bytes
	void shrReg32(Register reg); // 2 bytes
	void sarReg32(Register reg); // 2 bytes
	
	void rolReg32(Register reg); // 2 bytes
	void rorReg32(Register reg); // 2 bytes
	
	void shlImm32(Register reg, uint8_t bits); // 2 or 3 bytes	
	void shrImm32(Register reg, uint8_t bits); // 2 or 3 bytes
	void sarImm32(Register reg, uint8_t bits); // 2 or 3 bytes
	
	void rolImm32(Register reg, uint8_t bits); // 2 or 3 bytes
	void rorImm32(Register reg, uint8_t bits); // 2 or 3 bytes
	void rcrImm32(Register reg, uint8_t bits); // 2 or 3 bytes
	
	void andReg32(Register reg, Register other); // 2 bytes
	void andImm32(Register reg, uint32_t value); // 6 bytes
	void andMemImm32(Register base, uint32_t offset, uint32_t value); // 10+ bytes
	
	void orReg32(Register reg, Register other); // 2 bytes
	void orImm32(Register reg, uint32_t value); // 6 bytes
	void orMemReg32(Register base, uint32_t offset, Register reg); // 6+ bytes
	void orMemImm32(Register base, uint32_t offset, uint32_t value); // 10+ bytes
	
	void xorReg32(Register reg, Register other); // 2 bytes
	void xorImm32(Register reg, uint32_t value); // 6 bytes
	
	void notReg32(Register reg); // 2 bytes
	
	void callAbs(Register temp, uint64_t addr); // 12 bytes
	
	void jumpRel(uint32_t offset); // 2 or 5 bytes
	void jumpAbs(Register temp, uint64_t addr); // 12 bytes
	
	void jumpIfCarry(uint8_t offset); // 2 bytes
	void jumpIfNotCarry(uint8_t offset); // 2 bytes
	
	void jumpIfOverflow(uint8_t offset); // 2 bytes
	void jumpIfNotOverflow(uint8_t offset); // 2 bytes
	
	void jumpIfZero(uint8_t offset); // 2 bytes
	void jumpIfNotZero(uint8_t offset); // 2 bytes
	
	void jumpIfSign(uint8_t offset); // 2 bytes
	void jumpIfNotSign(uint8_t offset); // 2 bytes
	
	void jumpIfEqual(uint8_t offset); // 2 bytes
	void jumpIfNotEqual(uint8_t offset); // 2 bytes
	
	void jumpIfBelow(uint8_t offset); // 2 bytes
	
	void compareImm32(Register reg, uint32_t value); // 5 or 6 bytes
	
	void testReg32(Register a, Register b); // 2 bytes
	
	void bitTestMem32(Register base, uint32_t offset, uint8_t bit); // 8+ bytes
	void bitTestReg32(Register reg, uint8_t bit); // 4 bytes
	
	void bitTestSetMem32(Register base, uint32_t offset, uint8_t bit); // 8+ bytes
	void bitTestResetMem32(Register base, uint32_t offset, uint8_t bit); // 8+ bytes
	void bitTestResetReg32(Register reg, uint8_t bit); // 4 bytes
	
private:
	void reserve(size_t size);
	
	char *buffer;
	size_t offset;
	size_t capacity;
};
