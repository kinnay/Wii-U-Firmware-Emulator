
#include "hardware.h"
#include "emulator.h"

#include "common/logger.h"
#include "common/buffer.h"
#include "common/fileutils.h"


Hardware::Hardware(Emulator *emulator) :
	latte(emulator),
	dsp(emulator),
	
	pi(&emulator->physmem),
	aes(&emulator->physmem),
	sha(&emulator->physmem),
	nand(&emulator->physmem),
	gpu(&emulator->physmem),
	
	ohci00(&emulator->physmem, 0),
	ohci01(&emulator->physmem, 1),
	ohci1(&emulator->physmem, 2),
	ohci2(&emulator->physmem, 3),
	
	sdio0(&emulator->physmem, SDIOController::TYPE_SD),
	sdio1(&emulator->physmem, SDIOController::TYPE_WIFI),
	sdio2(&emulator->physmem, SDIOController::TYPE_MLC),
	sdio3(&emulator->physmem, SDIOController::TYPE_UNK)
	{}

void Hardware::reset() {
	latte.reset();
	pi.reset();
	
	exi.reset();
	ahmn.reset();
	mem.reset();
	aes.reset();
	sha.reset();
	nand.reset();
	gpu.reset();
	ai.reset();
	dsp.reset();
	
	ahci.reset();
	
	ehci0.reset();
	ehci1.reset();
	ehci2.reset();
	
	ohci00.reset();
	ohci01.reset();
	ohci1.reset();
	ohci2.reset();
	
	sdio0.reset();
	sdio1.reset();
	sdio2.reset();
	sdio3.reset();
}

template <>
uint32_t Hardware::read(uint32_t addr) {
	uint32_t masked = addr & ~0x800000;
	
	if (0xC000000 <= masked && masked < 0xC100000) return pi.read(masked);
	if (0xC200000 <= masked && masked < 0xC280000) return gpu.read(masked);
	if (0xD000000 <= masked && masked < 0xD001000) return latte.read(masked);
	if (0xD006800 <= masked && masked < 0xD006C00) return exi.read(masked - 0xD006800);
	if (0xD006C00 <= masked && masked < 0xD006E00) return ai.read(masked);
	if (0xD010000 <= masked && masked < 0xD020000) return nand.read(masked);
	if (0xD020000 <= masked && masked < 0xD030000) return aes.read(masked - 0xD020000);
	if (0xD030000 <= masked && masked < 0xD040000) return sha.read(masked - 0xD030000);
	if (0xD040000 <= masked && masked < 0xD050000) return ehci0.read(masked - 0xD040000);
	if (0xD050000 <= masked && masked < 0xD060000) return ohci00.read(masked - 0xD050000);
	if (0xD060000 <= masked && masked < 0xD070000) return ohci01.read(masked - 0xD060000);
	if (0xD070000 <= masked && masked < 0xD080000) return sdio0.read(masked - 0xD070000);
	if (0xD080000 <= masked && masked < 0xD090000) return sdio1.read(masked - 0xD080000);
	if (0xD0B0000 <= masked && masked < 0xD0B4000) return ahmn.read(masked);
	if (0xD100000 <= masked && masked < 0xD110000) return sdio2.read(masked - 0xD100000);
	if (0xD110000 <= masked && masked < 0xD120000) return sdio3.read(masked - 0xD110000);
	if (0xD120000 <= masked && masked < 0xD130000) return ehci1.read(masked - 0xD120000);
	if (0xD130000 <= masked && masked < 0xD140000) return ohci1.read(masked - 0xD130000);
	if (0xD140000 <= masked && masked < 0xD150000) return ehci2.read(masked - 0xD140000);
	if (0xD150000 <= masked && masked < 0xD160000) return ohci2.read(masked - 0xD150000);
	if (0xD160000 <= masked && masked < 0xD170000) return ahci.read(masked);
	
	if (0xD0000000 <= addr && addr < 0xD2000000) return gpu.read(addr);
	
	Logger::warning("Unknown physical memory read: 0x%08X", addr);
	return 0;
}

template <>
void Hardware::write(uint32_t addr, uint32_t value) {
	uint32_t masked = addr & ~0x800000;
	
	if (0xC000000 <= masked && masked < 0xC100000) pi.write(masked, value);
	else if (0xC200000 <= masked && masked < 0xC280000) gpu.write(masked, value);
	else if (0xD000000 <= masked && masked < 0xD001000) latte.write(masked, value);
	else if (0xD006800 <= masked && masked < 0xD006C00) exi.write(masked - 0xD006800, value);
	else if (0xD006C00 <= masked && masked < 0xD006E00) ai.write(masked, value);
	else if (0xD010000 <= masked && masked < 0xD020000) nand.write(masked, value);
	else if (0xD020000 <= masked && masked < 0xD030000) aes.write(masked - 0xD020000, value);
	else if (0xD030000 <= masked && masked < 0xD040000) sha.write(masked - 0xD030000, value);
	else if (0xD040000 <= masked && masked < 0xD050000) ehci0.write(masked - 0xD040000, value);
	else if (0xD050000 <= masked && masked < 0xD060000) ohci00.write(masked - 0xD050000, value);
	else if (0xD060000 <= masked && masked < 0xD070000) ohci01.write(masked - 0xD060000, value);
	else if (0xD070000 <= masked && masked < 0xD080000) sdio0.write(masked - 0xD070000, value);
	else if (0xD080000 <= masked && masked < 0xD090000) sdio1.write(masked - 0xD080000, value);
	else if (0xD0B0000 <= masked && masked < 0xD0B4000) ahmn.write(masked, value);
	else if (0xD100000 <= masked && masked < 0xD110000) sdio2.write(masked - 0xD100000, value);
	else if (0xD110000 <= masked && masked < 0xD120000) sdio3.write(masked - 0xD110000, value);
	else if (0xD120000 <= masked && masked < 0xD130000) ehci1.write(masked - 0xD120000, value);
	else if (0xD130000 <= masked && masked < 0xD140000) ohci1.write(masked - 0xD130000, value);
	else if (0xD140000 <= masked && masked < 0xD150000) ehci2.write(masked - 0xD140000, value);
	else if (0xD150000 <= masked && masked < 0xD160000) ohci2.write(masked - 0xD150000, value);
	else if (0xD160000 <= masked && masked < 0xD170000) ahci.write(masked, value);
	
	else if (0xD0000000 <= addr && addr < 0xD2000000) gpu.write(addr, value);
	
	else {
		Logger::warning("Unknown physical memory write: 0x%08X (0x%08X)", addr, value);
	}
}

template <>
uint16_t Hardware::read(uint32_t addr) {
	uint32_t masked = addr & ~0x800000;
	
	if (0xC280000 <= masked && masked < 0xC2C0000) return dsp.read(masked);
	if (0xD0B4000 <= masked && masked < 0xD0B4800) return mem.read(masked);
	
	Logger::warning("Unknown physical memory read: 0x%08X", addr);
	return 0;
}

template <>
void Hardware::write(uint32_t addr, uint16_t value) {
	uint32_t masked = addr & ~0x800000;
	
	if (0xC280000 <= masked && masked < 0xC2C0000) dsp.write(masked, value);
	else if (0xD0B4000 <= masked && masked < 0xD0B4800) mem.write(masked, value);
	else {
		Logger::warning("Unknown physical memory write: 0x%08X (0x%04X)", addr, value);
	}
}

void Hardware::update() {
	ai.update();
	dsp.update();
	gpu.update();
	latte.update();
	
	ohci00.update();
	ohci01.update();
	ohci1.update();
	ohci2.update();
	
	if (nand.check_interrupts()) latte.irq_arm.intsr_all |= 1 << 1;
	if (aes.check_interrupts()) latte.irq_arm.intsr_all |= 1 << 2;
	if (sha.check_interrupts()) latte.irq_arm.intsr_all |= 1 << 3;
	if (ohci00.check_interrupts()) latte.irq_arm.intsr_all |= 1 << 5;
	if (ohci01.check_interrupts()) latte.irq_arm.intsr_all |= 1 << 6;
	if (sdio0.check_interrupts()) latte.irq_arm.intsr_all |= 1 << 7;
	if (sdio1.check_interrupts()) latte.irq_arm.intsr_all |= 1 << 8;
	if (exi.check_interrupts()) latte.irq_arm.intsr_all |= 1 << 20;
	
	if (sdio2.check_interrupts()) latte.irq_arm.intsr_lt |= 1 << 0;
	if (sdio3.check_interrupts()) latte.irq_arm.intsr_lt |= 1 << 1;
	if (ohci1.check_interrupts()) latte.irq_arm.intsr_lt |= 1 << 3;
	if (ohci2.check_interrupts()) latte.irq_arm.intsr_lt |= 1 << 5;
	
	pi.set_irq(6, dsp.check_interrupts());
	pi.set_irq(23, gpu.check_interrupts());
	for (int i = 0; i < 3; i++) {
		pi.set_irq(i, 24, latte.irq_ppc[i].check_interrupts());
	}
}

bool Hardware::check_interrupts_arm() {
	return latte.irq_arm.check_interrupts();
}

bool Hardware::check_interrupts_ppc(int core) {
	return pi.check_interrupts(core);
}
