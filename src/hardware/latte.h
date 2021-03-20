
#pragma once

#include "hardware/i2c.h"
#include "hardware/ipc.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"

#include "common/buffer.h"

#include <map>

#include <cstdint>


class Emulator;
class PhysicalMemory;


class ASICController {
public:
	void reset();
	
	uint32_t read(uint32_t offset);
	void write(uint32_t offset, uint32_t value);
	
private:
	std::map<uint32_t, uint32_t> data;
};


class OTPController {
public:
	void reset();
	
	uint32_t read(int bank, int index);
	
private:
	Buffer buffer;
	uint32_t *data;
};


class LatteController {
public:
	enum Register {
		LT_TIMER = 0xD000010,
		LT_ALARM = 0xD000014,
		LT_WDG_CFG = 0xD00004C,
		LT_DMA_INT_STATUS = 0xD000050,
		LT_CPU_INT_STATUS = 0xD000054,
		LT_DBG_INT_STATUS = 0xD000058,
		LT_DBG_INT_ENABLE = 0xD00005C,
		LT_SRNPROT = 0xD000060,
		LT_BUSPROT = 0xD000064,
		LT_I2C_PPC_INT_MASK = 0xD000068,
		LT_I2C_PPC_INT_STATE = 0xD00006C,
		LT_AIP_PROT = 0xD000070,
		LT_AIP_IOCTRL = 0xD000074,
		LT_USB_RESET = 0xD000088,
		LT_DI_RESET = 0xD000180,
		LT_SPARE = 0xD000188,
		LT_BOOT0 = 0xD00018C,
		LT_CLOCKINFO = 0xD000190,
		LT_RESETS_COMPAT = 0xD000194,
		LT_IFPOWER = 0xD000198,
		LT_PLL_AI_EXT = 0xD0001D0,
		LT_IOPOWER = 0xD0001DC,
		LT_IOSTRENGTH_CTRL0 = 0xD0001E0,
		LT_IOSTRENGTH_CTRL1 = 0xD0001E4,
		LT_CLOCK_STRENGTH_CTRL = 0xD0001E8,
		LT_OTPCMD = 0xD0001EC,
		LT_OTPDATA = 0xD0001F0,
		LT_SI_CLOCK = 0xD000204,
		LT_ASICREV_ACR = 0xD000214,
		LT_DMA_INT_STATUS2 = 0xD0004A4,
		LT_CPU_INT_STATUS2 = 0xD0004A8,
		LT_D800500 = 0xD000500,
		LT_D800504 = 0xD000504,
		LT_OTPPROT = 0xD000510,
		LT_ASICREV_CCR = 0xD0005A0,
		LT_DEBUG = 0xD0005A4,
		LT_COMPAT_MEMCTRL_STATE = 0xD0005B0,
		LT_IOP2X = 0xD0005BC,
		LT_IOSTRENGTH_CTRL2 = 0xD0005C8,
		LT_IOSTRENGTH_CTRL3 = 0xD0005CC,
		LT_RESETS = 0xD0005E0,
		LT_RESETS_AHMN = 0xD0005E4,
		LT_SYSPLL_CFG = 0xD0005EC,
		LT_ABIF_CPLTL_OFFSET = 0xD000620,
		LT_ABIF_CPLTL_DATA = 0xD000624,
		LT_ABIF_CPLTL_CTRL = 0xD000628,
		LT_60XE_CFG  = 0xD000640,
		
		LT_GPIO_START = 0xD0000C0,
		LT_GPIO_END = 0xD000100,
		
		LT_I2C_PPC_START = 0xD000250,
		LT_I2C_PPC_END = 0xD000260,
		
		LT_IPC_START = 0xD000400,
		LT_IPC_END = 0xD000430,
		
		LT_IRQ_PPC_START = 0xD000440,
		LT_IRQ_PPC_END = 0xD000470,
		
		LT_IRQ_ARM_START = 0xD000470,
		LT_IRQ_ARM_END = 0xD000488,
		
		LT_GPIO2_START = 0xD000520,
		LT_GPIO2_END = 0xD000560,
		
		LT_I2C_START = 0xD000570,
		LT_I2C_END = 0xD000588
	};
	
	LatteController(Emulator *emulator);
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
	void reset();
	void update();
	
	IRQController irq_arm;
	IRQController irq_ppc[3];
	
private:
	void start_ppc();
	void reset_ppc();
	
	uint32_t timer;
	uint32_t alarm;
	uint32_t wdgcfg;
	uint32_t dbgintsts;
	uint32_t dbginten;
	uint32_t srnprot;
	uint32_t busprot;
	uint32_t aipprot;
	uint32_t aipctrl;
	uint32_t di_reset;
	uint32_t spare;
	uint32_t boot0;
	uint32_t clock_info;
	uint32_t resets_compat;
	uint32_t ifpower;
	uint32_t pll_aiext;
	uint32_t iopower;
	uint32_t iostrength_ctrl0;
	uint32_t iostrength_ctrl1;
	uint32_t clock_strength_ctrl;
	uint32_t otpcmd;
	uint32_t otpdata;
	uint32_t si_clock;
	uint32_t d800500;
	uint32_t d800504;
	uint32_t otpprot;
	uint32_t debug;
	uint32_t compat_memctrl_state;
	uint32_t iop2x;
	uint32_t iostrength_ctrl2;
	uint32_t iostrength_ctrl3;
	uint32_t resets;
	uint32_t resets_ahmn;
	uint32_t syspll_cfg;
	uint32_t asic_offs;
	uint32_t asic_ctrl;
	uint32_t cfg_60xe;
	
	ASICController asic;
	OTPController otp;
	IPCController ipc[3];
	
	SMCController smc;
	HDMIController hdmi;
	I2CController i2c;
	I2CController i2c_ppc;
	
	GPIOCommon gpio_common;
	GPIOLatte gpio_latte;
	GPIOController gpio;
	GPIOController gpio2;
	
	uint8_t key[16];
	uint8_t iv[16];
	
	Emulator *emulator;
	PhysicalMemory *physmem;
};
