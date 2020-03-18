
#include "hardware.h"
#include "processor.h"
#include "physicalmemory.h"
#include "common/logger.h"
#include "common/buffer.h"
#include "common/fileutils.h"
#include "common/endian.h"
#include <openssl/aes.h>
#include <cstring>


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


IPCController::IPCController(Hardware *hardware, int index) {
	this->hardware = hardware;
	this->index = index;
}

void IPCController::reset() {
	ppcmsg = 0;
	armmsg = 0;
	
	x1 = false;
	x2 = false;
	y1 = false;
	y2 = false;
	ix1 = false;
	ix2 = false;
	iy1 = false;
	iy2 = false;
}

uint32_t IPCController::read(uint32_t addr) {
	std::lock_guard<std::mutex> guard(mutex);
	
	switch (addr) {
		case LT_IPC_PPCMSG: return ppcmsg;
		case LT_IPC_PPCCTRL: return x1 | (y2 << 1) | (y1 << 2) | (x2 << 3) | (iy1 << 4) | (iy2 << 5);
		case LT_IPC_ARMMSG: return armmsg;
		case LT_IPC_ARMCTRL: return y1 | (x2 << 1) | (x1 << 2) | (y2 << 3) | (ix1 << 4) | (ix2 << 5);
	}
	
	Logger::warning("Unknown ipc read: 0x%X", addr);
	return 0;
}

void IPCController::write(uint32_t addr, uint32_t value) {
	std::lock_guard<std::mutex> guard(mutex);
	if (addr == LT_IPC_PPCMSG) ppcmsg = value;
	else if (addr == LT_IPC_PPCCTRL) {
		if (value & 1) x1 = true;
		if (value & 2) y2 = false;
		if (value & 4) y1 = false;
		if (value & 8) x2 = true;
		iy1 = value & 0x10;
		iy2 = value & 0x20;
	}
	else if (addr == LT_IPC_ARMMSG) armmsg = value;
	else if (addr == LT_IPC_ARMCTRL) {
		if (value & 1) y1 = true;
		if (value & 2) x2 = false;
		if (value & 4) x1 = false;
		if (value & 8) y2 = true;
		ix1 = value & 0x10;
		ix2 = value & 0x20;
	}
	else {
		Logger::warning("Unknown ipc write: 0x%X (0x%08X)", addr, value);
	}
}

void IPCController::update() {
	if ((x1 && ix1) || (x2 && ix2)) {
		hardware->trigger_irq_lt(Hardware::ARM, 31 - index * 2);
	}
	if ((y1 && iy1) || (y2 && iy2)) {
		hardware->trigger_irq_lt(Hardware::PPC0 + index, 30 - index * 2);
	}
}


LatteController::LatteController(
	Hardware *hardware, PhysicalMemory *physmem,
	Processor *ppc0, Processor *ppc1, Processor *ppc2
) :
	irq_arm(hardware, -1),
	irq_ppc {
		{hardware, 0},
		{hardware, 1},
		{hardware, 2}
	},
	i2c(hardware, &smc, false),
	i2c_ppc(hardware, &hdmi, true),
	gpio_latte(&hdmi),
	gpio(hardware, &gpio_common),
	gpio2(hardware, &gpio_latte),
	ipc {
		{hardware, 0},
		{hardware, 1},
		{hardware, 2}
	}
{
	this->hardware = hardware;
	this->physmem = physmem;
	ppc[0] = ppc0;
	ppc[1] = ppc1;
	ppc[2] = ppc2;
	
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
	
	gpio.reset();
	gpio2.reset();
	i2c.reset();
	asic.reset();
	otp.reset();
	
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
		irq_arm.trigger_irq_lt(12);
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
		ppc[i]->enable();
	}
}

void LatteController::reset_ppc() {
	for (int i = 0; i < 3; i++) {
		ppc[i]->disable();
	}
}

void LatteController::update() {
	timer++;
	if (timer == alarm) {
		irq_arm.trigger_irq_all(0);
	}
	
	for (int i = 0; i < 3; i++) {
		ipc[i].update();
	}
	
	i2c.update();
	i2c_ppc.update();
	gpio.update();
	gpio2.update();
	
	irq_arm.update();
	for (int i = 0; i < 3; i++) {
		irq_ppc[i].update();
	}
}


void RTCController::reset() {
	data = 0;
	offset = 0;
	words = 0;
	
	counter = 0;
	offtimer = 0;
	control0 = 0;
	control1 = 0;
	
	sramidx = 0;
}

uint32_t RTCController::read() {
	return data;
}

uint32_t RTCController::read_value(uint32_t offset) {
	switch (offset) {
		case 0x20000000: return counter;
		case 0x21000100: return offtimer;
		case 0x21000C00: return control0;
		case 0x21000D00: return control1;
	}
	
	Logger::warning("Unknown rtc read: 0x%08X", offset);
	return 0;
}

void RTCController::process_write(uint32_t offset, uint32_t value) {
	if (offset == 0x20000100) {
		sram[sramidx] = Endian::swap32(value);
		sramidx = (sramidx + 1) % 16;
	}
	else if (offset == 0x21000100) offtimer = value & 0x3FFFFFFF;
	else if (offset == 0x21000C00) control0 = value;
	else if (offset == 0x21000D00) control1 = value;
	else {
		Logger::warning("Unknown rtc write: 0x%08X (0x%08X)", offset, value);
	}
}

void RTCController::write(uint32_t value) {
	if (words > 0) {
		process_write(offset, value);
		words--;
	}
	else {
		if (value & 0x80000000) {
			offset = value & ~0x80000000;			
			if (offset == 0x20000100) {
				words = 16;
			}
			else {
				words = 1;
			}
		}
		else {
			data = read_value(value);
		}
	}
}


EXIController::EXIController(Hardware *hardware) {
	this->hardware = hardware;
}

void EXIController::reset() {
	csr = 0;
	data = 0;
	
	rtc.reset();
}

uint32_t EXIController::read(uint32_t addr) {
	switch (addr) {
		case EXI_CSR: return csr;
		case EXI_DATA: return data;
	}
	
	Logger::warning("Unknown exi memory read: 0x%X", addr);
	return 0;
}

void EXIController::write(uint32_t addr, uint32_t value) {
	if (addr == EXI_CSR) csr = value;
	else if (addr == EXI_CR) {
		if (value == 0x31) {
			data = rtc.read();
			hardware->trigger_irq_all(Hardware::ARM, 20);
		}
		else if (value == 0x35) {
			csr |= 4;
			rtc.write(data);
			hardware->trigger_irq_all(Hardware::ARM, 20);
		}
		else {
			Logger::warning("Unknown exi command: 0x%08X", value);
		}
	}
	else if (addr == EXI_DATA) data = value;
	else {
		Logger::warning("Unknown exi memory write: 0x%X (0x%08X)", addr, value);
	}
}


void AHMNController::reset() {
	mem0_config = 0;
	mem1_config = 0;
	mem2_config = 0;
	workaround = 0;
}

uint32_t AHMNController::read(uint32_t addr) {
	switch (addr) {
		case AHMN_MEM0_CONFIG: return mem0_config;
		case AHMN_MEM1_CONFIG: return mem1_config;
		case AHMN_MEM2_CONFIG: return mem2_config;
		case AHMN_TRANSFER_STATE: return 0;
		case AHMN_WORKAROUND: return workaround;
	}
	
	if (AHMN_MEM0_START <= addr && addr < AHMN_MEM0_END) return mem0[(addr - AHMN_MEM0_START) / 4];
	if (AHMN_MEM1_START <= addr && addr < AHMN_MEM1_END) return mem1[(addr - AHMN_MEM1_START) / 4];
	if (AHMN_MEM2_START <= addr && addr < AHMN_MEM2_END) return mem2[(addr - AHMN_MEM2_START) / 4];
	
	Logger::warning("Unknown ahmn read: 0x%X", addr);
	return 0;
}

void AHMNController::write(uint32_t addr, uint32_t value) {
	if (addr == AHMN_REVISION) {}
	else if (addr == AHMN_MEM0_CONFIG) mem0_config = value;
	else if (addr == AHMN_MEM1_CONFIG) mem1_config = value;
	else if (addr == AHMN_MEM2_CONFIG) mem2_config = value;
	else if (addr == AHMN_RDBI_MASK) {}
	else if (addr == AHMN_INTMSK) {}
	else if (addr == AHMN_INTSTS) {}
	else if (addr == AHMN_D8B0840) {}
	else if (addr == AHMN_D8B0844) {}
	else if (addr == AHMN_WORKAROUND) workaround = value;
	else if (AHMN_MEM0_START <= addr && addr < AHMN_MEM0_END) mem0[(addr - AHMN_MEM0_START) / 4] = value ^ 0x80000000;
	else if (AHMN_MEM1_START <= addr && addr < AHMN_MEM1_END) mem1[(addr - AHMN_MEM1_START) / 4] = value ^ 0x80000000;
	else if (AHMN_MEM2_START <= addr && addr < AHMN_MEM2_END) mem2[(addr - AHMN_MEM2_START) / 4] = value ^ 0x80000000;
	else {
		Logger::warning("Unknown ahmn write: 0x%X (0x%08X)", addr, value);
	}
}


AESController::AESController(Hardware *hardware, PhysicalMemory *physmem) {
	this->hardware = hardware;
	this->physmem = physmem;
}

void AESController::reset() {
	ctrl = 0;
	src = 0;
	dest = 0;
	
	memset(key, 0, sizeof(key));
	memset(iv, 0, sizeof(iv));
}

uint32_t AESController::read(uint32_t addr) {
	switch (addr) {
		case AES_CTRL: return ctrl;
	}
	
	Logger::warning("Unknown aes read: 0x%X", addr);
	return 0;
}

void AESController::write(uint32_t addr, uint32_t value) {
	if (addr == AES_CTRL) {
		ctrl = value;
		if (value >> 31) {
			ctrl = (ctrl & ~0x80000000) | 0xFFF;
			
			size_t size = ((value & 0xFFF) + 1) * 16;
			
			Buffer data = physmem->read(src, size);
			
			if (value & 0x10000000) {
				if (!(value & 0x1000)) {
					if (value & 0x8000000) AES_set_decrypt_key((uint8_t *)key, 128, &aeskey);
					else {
						AES_set_encrypt_key((uint8_t *)key, 128, &aeskey);
					}
				}
				
				if (value & 0x8000000) {
					AES_cbc_encrypt(data.get(), data.get(), size, &aeskey, (uint8_t *)iv, AES_DECRYPT);
				}
				else {
					AES_cbc_encrypt(data.get(), data.get(), size, &aeskey, (uint8_t *)iv, AES_ENCRYPT);
				}
			}
			
			physmem->write(dest, data);
			
			if (value & 0x40000000) {
				hardware->trigger_irq_all(Hardware::ARM, 2);
			}
		}
	}
	else if (addr == AES_SRC) src = value;
	else if (addr == AES_DEST) dest = value;
	else if (addr == AES_KEY) {
		for (int i = 0; i < 3; i++) {
			key[i] = key[i + 1];
		}
		key[3] = Endian::swap32(value);
	}
	else if (addr == AES_IV) {
		for (int i = 0; i < 3; i++) {
			iv[i] = iv[i + 1];
		}
		iv[3] = Endian::swap32(value);
	}
	else {
		Logger::warning("Unknown aes write: 0x%X (0x%08X)", addr, value);
	}
}


SHAController::SHAController(Hardware *hardware, PhysicalMemory *physmem) {
	this->hardware = hardware;
	this->physmem = physmem;
}

void SHAController::reset() {
	ctrl = 0;
	src = 0;
}

uint32_t SHAController::read(uint32_t addr) {
	switch (addr) {
		case SHA_CTRL: return ctrl;
		case SHA_H0: return sha1.h0;
		case SHA_H1: return sha1.h1;
		case SHA_H2: return sha1.h2;
		case SHA_H3: return sha1.h3;
		case SHA_H4: return sha1.h4;
	}
	
	Logger::warning("Unknown sha read: 0x%X", addr);
	return 0;
}

void SHAController::write(uint32_t addr, uint32_t value) {
	if (addr == SHA_CTRL) {
		ctrl = value & ~0x80000000;
		if (value & 0x80000000) {
			int blocks = (value & 0x3FF) + 1;
			for (int i = 0; i < blocks; i++) {
				char block[0x40];
				physmem->read(src, block, 0x40);
				sha1.update(block);
				src += 0x40;
			}
			
			if (value & 0x40000000) {
				hardware->trigger_irq_all(Hardware::ARM, 3);
			}
		}
	}
	else if (addr == SHA_SRC) src = value;
	else if (addr == SHA_H0) sha1.h0 = value;
	else if (addr == SHA_H1) sha1.h1 = value;
	else if (addr == SHA_H2) sha1.h2 = value;
	else if (addr == SHA_H3) sha1.h3 = value;
	else if (addr == SHA_H4) sha1.h4 = value;
	else {
		Logger::warning("Unknown sha write: 0x%X (0x%08X)", addr, value);
	}
}


void NANDBank::prepare(Hardware *hardware, PhysicalMemory *physmem, uint8_t *slc, uint8_t *slccmpt) {
	this->hardware = hardware;
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
		hardware->trigger_irq_all(Hardware::ARM, 1);
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


NANDController::NANDController(Hardware *hardware, PhysicalMemory *physmem) {
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
	
	main.prepare(hardware, physmem, slc, slccmpt);
	for (int i = 0; i < 8; i++) {
		banks[i].prepare(hardware, physmem, slc, slccmpt);
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


void EHCIPort::reset() {
	status = 0;
}

uint32_t EHCIPort::read_status() {
	return status;
}

void EHCIPort::write_status(uint32_t value) {
	status = value;
}


void EHCIController::reset() {
	usbcmd = 0x80000;
	usbsts = 0x1000;
	usbintr = 0;
	frindex = 0;
	ctrlsegment = 0;
	periodiclist = 0;
	asynclist = 0;
	configflag = 0;
	
	a4 = 0;
	
	for (int i = 0; i < 6; i++) {
		ports[i].reset();
	}
}

uint32_t EHCIController::read(uint32_t addr) {
	switch (addr) {
		case USBCMD: return usbcmd;
		case USBSTS: return usbsts;
		case USBINTR: return usbintr;
		case FRINDEX: return frindex;
		case CTRLDSSEGMENT: return ctrlsegment;
		case EHCI_A4: return a4;
	}
	
	if (PORTSC_START <= addr && addr < PORTSC_END) {
		int port = (addr - PORTSC_START) / 4;
		return ports[port].read_status();
	}
	
	Logger::warning("Unknown ehci read: 0x%X", addr);
	return 0;
}

void EHCIController::write(uint32_t addr, uint32_t value) {
	if (addr == USBCMD) {
		usbcmd = value & ~3;
		if (value & 2) {
			reset();
		}
		if (value & 1) {
			Logger::warning("EHCI command processing not implemented");
		}
	}
	else if (addr == USBSTS) usbsts &= ~value;
	else if (addr == USBINTR) usbintr = value;
	else if (addr == FRINDEX) frindex = value;
	else if (addr == PERIODICLISTBASE) periodiclist = value;
	else if (addr == ASYNCLISTADDR) asynclist = value;
	else if (addr == CONFIGFLAG) configflag = value;
	else if (addr == EHCI_A4) a4 = value;
	else if (addr == EHCI_B0) {}
	else if (addr == EHCI_B4) {}
	else if (addr == EHCI_CC) {}
	else if (PORTSC_START <= addr && addr < PORTSC_END) {
		int port = (addr - PORTSC_START) / 4;
		ports[port].write_status(value);
	}
	else {
		Logger::warning("Unknown ehci write: 0x%X (0x%08X)", addr, value);
	}
}


void AIController::reset() {
	timer = 0;
	
	control = 0;
	volume = 0;
	samples = 0;
}

void AIController::update() {
	int limit = control & 0x40 ? 60 : 40;
	
	timer++;
	if (timer == limit) {
		samples++;
		timer = 0;
	}
}

uint32_t AIController::read(uint32_t addr) {
	switch (addr) {
		case AI_CONTROL: return control;
		case AI_VOLUME: return volume;
		case AI_AISCNT: return samples;
	}
	Logger::warning("Unknown ai read: 0x%X", addr);
	return 0;
}

void AIController::write(uint32_t addr, uint32_t value) {
	if (addr == AI_CONTROL) {
		control = value;
		if (value & 0x20) {
			samples = 0;
		}
	}
	else if (addr == AI_VOLUME) volume = value;
	else {
		Logger::warning("Unknown ai write: 0x%X (0x%08X)", addr, value);
	}
}


DSPController::DSPController(Hardware *hardware) {
	this->hardware = hardware;
}

void DSPController::reset() {
	mailbox_in = 0;
	mailbox_out = 0x80000000;
	
	halted = true;
	int_status = false;
	int_mask = false;
	
	state = STATE_NEXT;
}

uint32_t DSPController::read(uint32_t addr) {
	switch (addr) {
		case DSP_MAILBOX_IN_H: return 0;
		case DSP_MAILBOX_OUT_H: return mailbox_out >> 16;
		case DSP_MAILBOX_OUT_L: return mailbox_out & 0xFFFF;
		case DSP_CONTROL_STATUS:
			return (halted << 2) | (int_status << 7) | (int_mask << 8);
	}
	Logger::warning("Unknown dsp read: 0x%X", addr);
	return 0;
}

void DSPController::write(uint32_t addr, uint32_t value) {
	if (addr == DSP_MAILBOX_IN_H) mailbox_in = (mailbox_in & 0xFFFF) | (value << 16);
	else if (addr == DSP_MAILBOX_IN_L) {
		mailbox_in = (mailbox_in & ~0xFFFF) | value;
		accept(mailbox_in);
	}
	else if (addr == DSP_CONTROL_STATUS) {
		halted = value & 4;
		int_mask = value & 0x100;
		
		if (value & 0x80) {
			int_status = false;
		}
		if (value & 0x801) reset();
	}
	else {
		Logger::warning("Unknown dsp write: 0x%X (0x%08X)", addr, value);
	}
}

void DSPController::update() {
	if (int_status && int_mask) {
		hardware->trigger_irq_pi(Hardware::PPC, 6);
	}
}

void DSPController::send_mail(MessageOut message) {
	mailbox_out = message;
	int_status = true;
}

void DSPController::accept(uint32_t mail) {
	if (state == STATE_NEXT) {
		message = mail;
		state = STATE_ARGUMENT;
	}
	else {
		process(message, mail);
		state = STATE_NEXT;
	}
}

void DSPController::process(uint32_t message, uint32_t arg) {
	if (message == 0x80F3D001) {
		if (arg & 0x10) {
			send_mail(DSP_INIT);
		}
	}
	Logger::warning("Unknown dsp message: 0x%08X 0x%08X", message, arg);
}


Hardware::Hardware(
	PhysicalMemory *physmem, Processor *ppc0,
	Processor *ppc1, Processor *ppc2
) : latte(this, physmem, ppc0, ppc1, ppc2),
	exi(this),
	aes(this, physmem),
	sha(this, physmem),
	nand(this, physmem),
	sdio0(this, physmem, SDIOController::TYPE_SD),
	sdio1(this, physmem, SDIOController::TYPE_WIFI),
	sdio2(this, physmem, SDIOController::TYPE_MLC),
	sdio3(this, physmem, SDIOController::TYPE_UNK),
	ohci00(this, physmem, 0),
	ohci01(this, physmem, 1),
	ohci1(this, physmem, 2),
	ohci2(this, physmem, 3),
	pi(this, physmem),
	gpu(this, physmem),
	dsp(this)
	{}

void Hardware::reset() {
	latte.reset();
	exi.reset();
	ahmn.reset();
	mem.reset();
	aes.reset();
	nand.reset();
	gpu.reset();
	dsp.reset();
	ai.reset();
	
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
	
	pi.reset();
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
	pi.update();
	
	latte.update();
	gpu.update();
	dsp.update();
	ai.update();
	
	ohci00.update();
	ohci01.update();
	ohci1.update();
	ohci2.update();
	
	sdio0.update();
	sdio1.update();
	sdio2.update();
	sdio3.update();
}

void Hardware::trigger_irq_all(int core, int irq) {
	if (core == ARM) latte.irq_arm.trigger_irq_all(irq);
	else if (core == PPC) {
		for (int i = 0; i < 3; i++) {
			latte.irq_ppc[i].trigger_irq_all(irq);
		}
	}
	else {
		latte.irq_ppc[core - PPC0].trigger_irq_all(irq);
	}
}

void Hardware::trigger_irq_lt(int core, int irq) {
	if (core == ARM) latte.irq_arm.trigger_irq_lt(irq);
	else if (core == PPC) {
		for (int i = 0; i < 3; i++) {
			latte.irq_ppc[i].trigger_irq_lt(irq);
		}
	}
	else {
		latte.irq_ppc[core - PPC0].trigger_irq_lt(irq);
	}
}

void Hardware::trigger_irq_pi(int core, int irq) {
	if (core == PPC) {
		for (int i = 0; i < 3; i++) {
			pi.trigger_irq(i, irq);
		}
	}
	else {
		pi.trigger_irq(core - PPC0, irq);
	}
}

bool Hardware::check_interrupts_arm() {
	return latte.irq_arm.check_interrupts();
}

bool Hardware::check_interrupts_ppc(int core) {
	return pi.check_interrupts(core);
}
