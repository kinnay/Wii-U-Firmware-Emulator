
#pragma once

#include "hardware/ahci.h"
#include "hardware/gpio.h"
#include "hardware/gpu.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "hardware/mem.h"
#include "hardware/ohci.h"
#include "hardware/pi.h"
#include "hardware/sdio.h"
#include "hardware/usb.h"
#include "common/logger.h"
#include "common/buffer.h"
#include "common/sha1.h"
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <openssl/aes.h>
#include <unistd.h>
#include <map>
#include <vector>
#include <mutex>
#include <cstdint>


class PhysicalMemory;
class Processor;


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


class IPCController {
public:
	enum Register {
		LT_IPC_PPCMSG = 0,
		LT_IPC_PPCCTRL = 4,
		LT_IPC_ARMMSG = 8,
		LT_IPC_ARMCTRL = 0xC
	};
	
	IPCController(Hardware *hardware, int index);
	
	void reset();
	void update();
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
private:
	std::mutex mutex;
	
	Hardware *hardware;
	int index;
	
	uint32_t ppcmsg;
	uint32_t armmsg;
	
	bool x1, x2, y1, y2;
	bool ix1, ix2, iy1, iy2;
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
	
	LatteController(
		Hardware *hardware, PhysicalMemory *physmem,
		Processor *ppc0, Processor *ppc1, Processor *ppc2
	);
	
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
	
	SMCController smc;
	HDMIController hdmi;
	I2CController i2c;
	I2CController i2c_ppc;
	
	ASICController asic;
	OTPController otp;
	IPCController ipc[3];
	
	GPIOCommon gpio_common;
	GPIOLatte gpio_latte;
	GPIOController gpio;
	GPIOController gpio2;
	
	uint8_t key[16];
	uint8_t iv[16];
	
	Hardware *hardware;
	PhysicalMemory *physmem;
	Processor *ppc[3];
};


class RTCController {
public:
	void reset();
	
	uint32_t read();
	void write(uint32_t value);
	
private:
	uint32_t read_value(uint32_t offset);
	void process_write(uint32_t offset, uint32_t value);
	
	int words;
	uint32_t offset;
	uint32_t data;
	
	uint32_t counter;
	uint32_t offtimer;
	uint32_t control0;
	uint32_t control1;
	
	uint32_t sram[16];
	int sramidx;
};


class EXIController {
public:
	enum Register {
		EXI_CSR = 0,
		EXI_CR = 0xC,
		EXI_DATA = 0x10
	};
	
	EXIController(Hardware *hardware);
	
	void reset();
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
private:
	Hardware *hardware;
	
	RTCController rtc;
	
	uint32_t csr;
	uint32_t data;
};


class AHMNController {
public:
	enum Register {
		AHMN_REVISION = 0xD0B0010,
		AHMN_MEM0_CONFIG = 0xD0B0800,
		AHMN_MEM1_CONFIG = 0xD0B0804,
		AHMN_MEM2_CONFIG = 0xD0B0808,
		AHMN_RDBI_MASK = 0xD0B080C,
		AHMN_INTMSK = 0xD0B0820,
		AHMN_INTSTS = 0xD0B0824,
		AHMN_D8B0840 = 0xD0B0840,
		AHMN_D8B0844 = 0xD0B0844,
		AHMN_TRANSFER_STATE = 0xD0B0850,
		AHMN_WORKAROUND = 0xD0B0854,
		
		AHMN_MEM0_START = 0xD0B0900,
		AHMN_MEM0_END = 0xD0B0980,
		
		AHMN_MEM1_START = 0xD0B0A00,
		AHMN_MEM1_END = 0xD0B0C00,
		
		AHMN_MEM2_START = 0xD0B0C00,
		AHMN_MEM2_END = 0xD0B1000
	};
	
	void reset();
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
private:
	uint32_t mem0_config;
	uint32_t mem1_config;
	uint32_t mem2_config;
	uint32_t workaround;
	
	uint32_t mem0[0x20];
	uint32_t mem1[0x80];
	uint32_t mem2[0x100];
};


class AESController {
public:
	enum Register {
		AES_CTRL = 0,
		AES_SRC = 4,
		AES_DEST = 8,
		AES_KEY = 0xC,
		AES_IV = 0x10
	};
	
	AESController(Hardware *hardware, PhysicalMemory *physmem);
	
	void reset();
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
private:
	uint32_t ctrl;
	uint32_t src;
	uint32_t dest;
	uint32_t key[4];
	uint32_t iv[4];
	
	::AES_KEY aeskey;
	
	Hardware *hardware;
	PhysicalMemory *physmem;
};


class SHAController {
public:
	enum Register {
		SHA_CTRL = 0,
		SHA_SRC = 4,
		SHA_H0 = 8,
		SHA_H1 = 0xC,
		SHA_H2 = 0x10,
		SHA_H3 = 0x14,
		SHA_H4 = 0x18
	};
	
	SHAController(Hardware *hardware, PhysicalMemory *physmem);
	
	void reset();
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
private:
	uint32_t ctrl;
	uint32_t src;
	
	Hardware *hardware;
	PhysicalMemory *physmem;
	
	SHA1 sha1;
};


class NANDBank {
public:
	enum Register {
		NAND_CTRL = 0,
		NAND_CONFIG = 4,
		NAND_ADDR1 = 8,
		NAND_ADDR2 = 0xC,
		NAND_DATABUF = 0x10,
		NAND_ECCBUF = 0x14
	};
	
	void prepare(Hardware *hardware, PhysicalMemory *physmem, uint8_t *slc, uint8_t *slccmpt);
	void reset();
	
	void set_bank(bool cmpt);
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
	void process_config();
	
private:
	void parse_addr(int flags);
	
	void process_ctrl(uint32_t value);
	void process_command(int command, int length);
	
	uint32_t ctrl;
	uint32_t config;
	uint32_t addr1;
	uint32_t addr2;
	uint32_t databuf;
	uint32_t eccbuf;
	
	uint32_t pagenum;
	uint32_t pageoff;
	
	uint8_t *slc;
	uint8_t *slccmpt;
	
	uint8_t *file;
	
	Hardware *hardware;
	PhysicalMemory *physmem;
};


class NANDController {
public:
	enum Register {
		NAND_BANK = 0xD010018,
		NAND_BANK_CTRL = 0xD010030,
		NAND_INT_MASK = 0xD010034,
		
		NAND_MAIN_START = 0xD010000,
		NAND_MAIN_END = 0xD010018,
		
		NAND_BANKS_START = 0xD010040,
		NAND_BANKS_END = 0xD010100
	};
	
	NANDController(Hardware *hardware, PhysicalMemory *physmem);
	~NANDController();
	
	void reset();
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
private:
	uint32_t bank_ctrl;
	
	NANDBank main;
	NANDBank banks[8];
	
	uint8_t *slc;
	uint8_t *slccmpt;
};


class EHCIPort {
public:
	void reset();
	
	uint32_t read_status();
	void write_status(uint32_t value);
	
private:
	uint32_t status;
};


class EHCIController {
public:
	enum Register {
		USBCMD = 0,
		USBSTS = 4,
		USBINTR = 8,
		FRINDEX = 0xC,
		CTRLDSSEGMENT = 0x10,
		PERIODICLISTBASE = 0x14,
		ASYNCLISTADDR = 0x18,
		CONFIGFLAG = 0x40,
		
		PORTSC_START = 0x54,
		PORTSC_END = 0x6C,
		
		EHCI_A4 = 0xA4,
		EHCI_B0 = 0xB0,
		EHCI_B4 = 0xB4,
		EHCI_CC = 0xCC
	};
	
	void reset();
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
private:
	uint32_t usbcmd;
	uint32_t usbsts;
	uint32_t usbintr;
	uint32_t frindex;
	uint32_t ctrlsegment;
	uint32_t periodiclist;
	uint32_t asynclist;
	uint32_t configflag;
	uint32_t a4;
	
	EHCIPort ports[6];
};


class AIController {
public:
	enum Register {
		AI_CONTROL = 0xD006C00,
		AI_VOLUME = 0xD006C04,
		AI_AISCNT = 0xD006C08,
		AI_AIIT = 0xD006C0C
	};
	
	void reset();
	void update();
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
private:
	int timer;
	
	uint32_t control;
	uint32_t volume;
	uint32_t samples;
};


class DSPController {
public:
	enum Register {
		DSP_MAILBOX_IN_H = 0xC280000,
		DSP_MAILBOX_IN_L = 0xC280002,
		DSP_MAILBOX_OUT_H = 0xC280004,
		DSP_MAILBOX_OUT_L = 0xC280006,
		DSP_CONTROL_STATUS = 0xC28000A
	};
	
	DSPController(Hardware *hardware);
	
	void reset();
	void update();
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
private:
	enum MessageOut {
		DSP_INIT = 0xDCD10000
	};
	
	enum State {
		STATE_NEXT,
		STATE_ARGUMENT
	};
	
	void accept(uint32_t mail);
	void process(uint32_t message, uint32_t arg);
	
	void send_mail(MessageOut message);
	
	uint32_t mailbox_in;
	uint32_t mailbox_out;
	
	bool halted;
	bool int_status;
	bool int_mask;
	
	uint32_t message;
	State state;
	
	Hardware *hardware;
};


class Hardware {
public:
	enum Core {
		ARM,
		PPC,
		PPC0,
		PPC1,
		PPC2
	};
	
	Hardware(
		PhysicalMemory *physmem, Processor *ppc0,
		Processor *ppc1, Processor *ppc2
	);
	
	template <class T>
	T read(uint32_t addr) {
		Logger::warning("Unknown physical memory read: 0x%08X", addr);
		return 0;
	}
	
	template <class T>
	void write(uint32_t addr, T value) {
		Logger::warning("Unknown physical memory write: 0x%08X", addr);
	}
	
	void reset();
	void update();
	
	void trigger_irq_all(int core, int irq);
	void trigger_irq_lt(int core, int irq);
	void trigger_irq_pi(int core, int irq);
	
	bool check_interrupts_arm();
	bool check_interrupts_ppc(int core);
	
	PIController pi;
	
private:
	LatteController latte;
	
	EXIController exi;
	AHMNController ahmn;
	MEMController mem;
	AESController aes;
	SHAController sha;
	NANDController nand;
	GPUController gpu;
	AIController ai;
	DSPController dsp;
	
	AHCIController ahci;
	
	EHCIController ehci0;
	EHCIController ehci1;
	EHCIController ehci2;
	
	OHCIController ohci00;
	OHCIController ohci01;
	OHCIController ohci1;
	OHCIController ohci2;
	
	SDIOController sdio0;
	SDIOController sdio1;
	SDIOController sdio2;
	SDIOController sdio3;
};


template <> uint16_t Hardware::read<uint16_t>(uint32_t addr);
template <> uint32_t Hardware::read<uint32_t>(uint32_t addr);

template <> void Hardware::write<uint16_t>(uint32_t addr, uint16_t value);
template <> void Hardware::write<uint32_t>(uint32_t addr, uint32_t value);
