
#include "hardware/latte.h"

#include "common/endian.h"
#include "common/fileutils.h"

#include "emulator.h"


void ASICController::reset() {
	data.clear();
}

uint32_t ASICController::read(uint32_t offset) {
	return data[offset];
}

void ASICController::write(uint32_t offset, uint32_t value) {
	data[offset] = value;
}


void OTPController::reset() {
	buffer = FileUtils::load("files/otp.bin");
	data = (uint32_t *)buffer.get();
}

uint32_t OTPController::read(int bank, int index) {
	return Endian::swap32(data[bank * 0x20 + index]);
}


LatteController::LatteController(Emulator *emulator) :
	i2c(&smc, false),
	i2c_ppc(&hdmi, true),
	gpio_latte(&hdmi),
	gpio(&gpio_common),
	gpio2(&gpio_latte)
{
	this->emulator = emulator;
	this->physmem = &emulator->physmem;
	
	memset(iv, 0, sizeof(iv));
	
	Buffer data = FileUtils::load("files/espresso_key.bin");
	if (data.size() != 16) {
		runtime_error("Espresso key has incorrect size");
	}	
	memcpy(key, data.get(), 16);
}

void LatteController::reset() {
	timer = 0;
	alarm = 0;
	wdgcfg = 0;
	dbgintsts = 0;
	dbginten = 0;
	srnprot = 0;
	busprot = 0;
	aipprot = 0;
	aipctrl = 0;
	di_reset = 0;
	spare = 0;
	boot0 = 0;
	clock_info = 0;
	resets_compat = 0;
	ifpower = 0;
	pll_aiext = 0;
	iopower = 0;
	iostrength_ctrl0 = 0;
	iostrength_ctrl1 = 0;
	clock_strength_ctrl = 0;
	otpcmd = 0;
	otpdata = 0;
	si_clock = 0;
	d800500 = 0;
	d800504 = 0;
	otpprot = 0xFFFFFFFF;
	debug = 0;
	compat_memctrl_state = 0;
	iop2x = 0;
	iostrength_ctrl2 = 0;
	iostrength_ctrl3 = 0;
	resets = 0;
	resets_ahmn = 0;
	syspll_cfg = 0;
	asic_offs = 0;
	asic_ctrl = 0;
	cfg_60xe = 0;
	
	irq_arm.reset();
	for (int i = 0; i < 3; i++) {
		irq_ppc[i].reset();
	}
	
	i2c.reset();
	i2c_ppc.reset();
	asic.reset();
	otp.reset();
	gpio.reset();
	gpio2.reset();
	
	for (int i = 0; i < 3; i++) {
		ipc[i].reset();
	}
}

uint32_t LatteController::read(uint32_t addr) {
	switch (addr) {
		case LT_TIMER: return timer;
		case LT_WDG_CFG: return wdgcfg;
		case LT_DBG_INT_STATUS: return dbgintsts;
		case LT_SRNPROT: return srnprot;
		case LT_BUSPROT: return busprot;
		case LT_I2C_PPC_INT_MASK: return i2c_ppc.read(I2CController::I2C_INT_MASK);
		case LT_I2C_PPC_INT_STATE: return i2c_ppc.read(I2CController::I2C_INT_STATE);
		case LT_AIP_PROT: return aipprot;
		case LT_AIP_IOCTRL: return aipctrl;
		case LT_DI_RESET: return di_reset;
		case LT_SPARE: return spare;
		case LT_BOOT0: return boot0;
		case LT_CLOCKINFO: return clock_info;
		case LT_RESETS_COMPAT: return resets_compat;
		case LT_IFPOWER: return ifpower;
		case LT_PLL_AI_EXT: return pll_aiext;
		case LT_IOPOWER: return iopower;
		case LT_IOSTRENGTH_CTRL0: return iostrength_ctrl0;
		case LT_IOSTRENGTH_CTRL1: return iostrength_ctrl1;
		case LT_CLOCK_STRENGTH_CTRL: return clock_strength_ctrl;
		case LT_OTPCMD: return otpcmd;
		case LT_OTPDATA: return otpdata;
		case LT_SI_CLOCK: return si_clock;
		case LT_ASICREV_ACR: return 0x21;
		case LT_D800500: return d800500;
		case LT_D800504: return d800504;
		case LT_OTPPROT: return otpprot;
		case LT_ASICREV_CCR: return 0xCAFE0060;
		case LT_DEBUG: return debug;
		case LT_COMPAT_MEMCTRL_STATE: return compat_memctrl_state;
		case LT_IOP2X: return iop2x;
		case LT_IOSTRENGTH_CTRL2: return iostrength_ctrl2;
		case LT_IOSTRENGTH_CTRL3: return iostrength_ctrl3;
		case LT_RESETS: return resets;
		case LT_RESETS_AHMN: return resets_ahmn;
		case LT_SYSPLL_CFG: return syspll_cfg;
		case LT_ABIF_CPLTL_DATA: return asic.read(asic_offs);
		case LT_ABIF_CPLTL_CTRL: return asic_ctrl;
		case LT_60XE_CFG: return cfg_60xe;
	}
	
	if (LT_IRQ_PPC_START <= addr && addr < LT_IRQ_PPC_END) {
		addr -= LT_IRQ_PPC_START;
		return irq_ppc[addr / 0x10].read(addr % 0x10);
	}
	if (LT_IRQ_ARM_START <= addr && addr < LT_IRQ_ARM_END) return irq_arm.read(addr - LT_IRQ_ARM_START);
	if (LT_GPIO_START <= addr && addr < LT_GPIO_END) return gpio.read(addr - LT_GPIO_START);
	if (LT_GPIO2_START <= addr && addr < LT_GPIO2_END) return gpio2.read(addr - LT_GPIO2_START);
	if (LT_I2C_START <= addr && addr < LT_I2C_END) return i2c.read(addr - LT_I2C_START);
	if (LT_I2C_PPC_START <= addr && addr < LT_I2C_PPC_END) return i2c_ppc.read(addr - LT_I2C_PPC_START);
	if (LT_IPC_START <= addr && addr < LT_IPC_END) {
		addr -= LT_IPC_START;
		return ipc[addr / 0x10].read(addr % 0x10);
	}
	
	Logger::warning("Unknown latte memory read: 0x%X", addr);
	return 0;
}

void LatteController::write(uint32_t addr, uint32_t value) {
	if (addr == LT_TIMER) timer = value;
	else if (addr == LT_ALARM) alarm = value;
	else if (addr == LT_WDG_CFG) wdgcfg = value;
	else if (addr == LT_DMA_INT_STATUS) {}
	else if (addr == LT_CPU_INT_STATUS) {}
	else if (addr == LT_DBG_INT_STATUS) dbgintsts = value & dbginten;
	else if (addr == LT_DBG_INT_ENABLE) dbginten = value;
	else if (addr == LT_SRNPROT) srnprot = value;
	else if (addr == LT_BUSPROT) busprot = value;
	else if (addr == LT_I2C_PPC_INT_MASK) i2c_ppc.write(I2CController::I2C_INT_MASK, value);
	else if (addr == LT_I2C_PPC_INT_STATE) i2c_ppc.write(I2CController::I2C_INT_STATE, value);
	else if (addr == LT_AIP_PROT) aipprot = value;
	else if (addr == LT_AIP_IOCTRL) aipctrl = value;
	else if (addr == LT_USB_RESET) {}
	else if (addr == LT_DI_RESET) di_reset = value;
	else if (addr == LT_SPARE) spare = value;
	else if (addr == LT_BOOT0) boot0 = value;
	else if (addr == LT_CLOCKINFO) clock_info = value;
	else if (addr == LT_RESETS_COMPAT) {
		uint32_t changed = resets_compat ^ value;
		uint32_t cleared = changed & (resets_compat & ~value);
		uint32_t set = changed & (~resets_compat & value);
		if (set & 0x10) {
			start_ppc();
		}
		if (cleared & 0x10) {
			reset_ppc();
		}
		resets_compat = value;
	}
	else if (addr == LT_IFPOWER) ifpower = value;
	else if (addr == LT_PLL_AI_EXT) pll_aiext = value;
	else if (addr == LT_IOPOWER) iopower = value;
	else if (addr == LT_IOSTRENGTH_CTRL0) iostrength_ctrl0 = value;
	else if (addr == LT_IOSTRENGTH_CTRL1) iostrength_ctrl1 = value;
	else if (addr == LT_CLOCK_STRENGTH_CTRL) clock_strength_ctrl = value;
	else if (addr == LT_OTPCMD) {
		otpcmd = value;
		if (value >> 31) {
			int bank = (value >> 8) & 7;
			int offset = value & 0x1F;
			int protbit = bank * 4 + offset / 8;
			if (otpprot & (1 << protbit)) {
				otpdata = otp.read(bank, offset);
			}
			else {
				Logger::warning("OTP data is protected");
				otpdata = 0;
			}
		}
	}
	else if (addr == LT_SI_CLOCK) si_clock = value;
	else if (addr == LT_DMA_INT_STATUS2) {}
	else if (addr == LT_CPU_INT_STATUS2) {}
	else if (addr == LT_D800500) d800500 = value;
	else if (addr == LT_D800504) d800504 = value;
	else if (addr == LT_OTPPROT) otpprot = value;
	else if (addr == LT_DEBUG) debug = value;
	else if (addr == LT_COMPAT_MEMCTRL_STATE) compat_memctrl_state = value;
	else if (addr == LT_IOP2X) {
		iop2x = value | 4;
		irq_arm.intsr_lt |= 1 << 12;
	}
	else if (addr == LT_IOSTRENGTH_CTRL2) iostrength_ctrl2 = value;
	else if (addr == LT_IOSTRENGTH_CTRL3) iostrength_ctrl3 = value;
	else if (addr == LT_RESETS) resets = value;
	else if (addr == LT_RESETS_AHMN) resets_ahmn = value;
	else if (addr == LT_SYSPLL_CFG) syspll_cfg = value;
	else if (addr == LT_ABIF_CPLTL_OFFSET) asic_offs = value;
	else if (addr == LT_ABIF_CPLTL_DATA) asic.write(asic_offs, value);
	else if (addr == LT_ABIF_CPLTL_CTRL) asic_ctrl = value;
	else if (addr == LT_60XE_CFG) cfg_60xe = value;
	
	else if (LT_IRQ_PPC_START <= addr && addr < LT_IRQ_PPC_END) {
		addr -= LT_IRQ_PPC_START;
		irq_ppc[addr / 0x10].write(addr % 0x10, value);
	}
	else if (LT_IRQ_ARM_START <= addr && addr < LT_IRQ_ARM_END) irq_arm.write(addr - LT_IRQ_ARM_START, value);
	else if (LT_GPIO_START <= addr && addr < LT_GPIO_END) gpio.write(addr - LT_GPIO_START, value);
	else if (LT_GPIO2_START <= addr && addr < LT_GPIO2_END) gpio2.write(addr - LT_GPIO2_START, value);
	else if (LT_I2C_START <= addr && addr < LT_I2C_END) i2c.write(addr - LT_I2C_START, value);
	else if (LT_I2C_PPC_START <= addr && addr < LT_I2C_PPC_END) i2c_ppc.write(addr - LT_I2C_PPC_START, value);
	else if (LT_IPC_START <= addr && addr < LT_IPC_END) {
		addr -= LT_IPC_START;
		ipc[addr / 0x10].write(addr % 0x10, value);
	}
	
	else {
		Logger::warning("Unknown latte memory write: 0x%X (0x%08X)", addr, value);
	}
}

void LatteController::start_ppc() {
	uint32_t size = physmem->read<uint32_t>(0x080000AC);
	Buffer data = physmem->read(0x08000100, size);
	
	AES_KEY aeskey;
	AES_set_decrypt_key(key, 128, &aeskey);
	AES_cbc_encrypt(data.get(), data.get(), size, &aeskey, iv, AES_DECRYPT);
	
	physmem->write(0x08000100, data);
	
	for (int i = 0; i < 3; i++) {
		emulator->ppc[i].enable();
	}
}

void LatteController::reset_ppc() {
	for (int i = 0; i < 3; i++) {
		emulator->ppc[i].disable();
	}
}

void LatteController::update() {
	timer++;
	if (timer == alarm) {
		irq_arm.intsr_all |= 1 << 0;
	}
	
	gpio.update();
	gpio2.update();
	
	if (gpio.check_interrupts(false) || gpio2.check_interrupts(false)) {
		irq_arm.intsr_all |= 1 << 11;
	}
	if (gpio.check_interrupts(true) || gpio2.check_interrupts(true)) {
		irq_ppc[0].intsr_all |= 1 << 10;
		irq_ppc[1].intsr_all |= 1 << 10;
		irq_ppc[2].intsr_all |= 1 << 10;
	}
	
	if (i2c.check_interrupts()) irq_arm.intsr_lt |= 1 << 14;
	if (i2c_ppc.check_interrupts()) {
		irq_ppc[0].intsr_lt |= 1 << 13;
		irq_ppc[1].intsr_lt |= 1 << 13;
		irq_ppc[2].intsr_lt |= 1 << 13;
	}
	
	for (int i = 0; i < 3; i++) {
		if (ipc[i].check_interrupts_arm()) {
			irq_arm.intsr_lt |= 1 << (31 - i * 2);
		}
		if (ipc[i].check_interrupts_ppc()) {
			irq_ppc[i].intsr_lt |= 1 << (30 - i * 2);
		}
	}
}
