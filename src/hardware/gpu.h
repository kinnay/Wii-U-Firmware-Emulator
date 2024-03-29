
#pragma once

#include <vector>

#include <cstdint>


class PhysicalMemory;


// Display controller
class DCController {
public:
	enum Register {
		D1CRTC_FORCE_COUNT_NOW_CNTL = 0x70,
		D1CRTC_STATUS = 0x9C,
		D1CRTC_INTERRUPT_CONTROL = 0xDC,
		
		// Deep flip queue
		D1GRPH_ENABLE = 0x100,
		D1GRPH_CONTROL = 0x104,
		D1GRPH_PRIMARY_SURFACE_ADDRESS = 0x110,
		D1GRPH_SECONDARY_SURFACE_ADDRESS = 0x118,
		D1GRPH_DFQ_CONTROL = 0x150,
		D1GRPH_DFQ_STATUS = 0x154,
		
		// Overlay
		D1OVL_ENABLE = 0x180,
		D1OVL_CONTROL1 = 0x184,
		D1OVL_SURFACE_ADDRESS = 0x190,
		
		DC_LUT_PWL_DATA = 0x490,
		DC_LUT_AUTOFILL = 0x4A0
	};
	
	void reset();
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
	uint32_t crtc_interrupt_control;
	
private:
	uint32_t grph_enable;
	uint32_t grph_control;
	uint32_t grph_primary_surface_addr;
	uint32_t grph_secondary_surface_addr;
	uint32_t grph_dfq_status;
	
	uint32_t ovl_enable;
	uint32_t ovl_control1;
	uint32_t ovl_surface_addr;

	uint32_t lut_autofill;
};


class PM4Processor {
public:
	PM4Processor(PhysicalMemory *physmem);
	
	void reset();
	void process(uint32_t value);
	bool check_interrupts();
	
private:
	enum Opcode {
		PM4_CONTEXT_CONTROL = 0x28,
		PM4_DRAW_INDEX_AUTO = 0x2D,
		PM4_INDIRECT_BUFFER = 0x32,
		PM4_MEM_WRITE = 0x3D,
		PM4_ME_INITIALIZE = 0x44,
		PM4_EVENT_WRITE = 0x46,
		PM4_EVENT_WRITE_EOP = 0x47,
		PM4_SET_CONFIG_REG = 0x68,
		PM4_SET_CONTEXT_REG = 0x69
	};

	enum State {
		STATE_NEXT,
		STATE_ARGUMENTS
	};
	
	void execute();
	
	PhysicalMemory *physmem;
	
	State state;
	
	std::vector<uint32_t> args;
	int argnum;
	
	bool retired;
};


// Command processor
class CPController {
public:
	enum Register  {
		// Ring buffer
		CP_RB_BASE = 0xC20C100,
		CP_RB_CNTL = 0xC20C104,
		CP_RB_RPTR_ADDR = 0xC20C10C,
		CP_RB_RPTR = 0xC20C110,
		CP_RB_WPTR = 0xC20C114,
		CP_RB_WPTR_DELAY = 0xC20C118,
		CP_RB_RPTR_WR = 0xC20C11C,
		
		// Prefetch parser
		CP_PFP_UCODE_ADDR = 0xC20C150,
		CP_PFP_UCODE_DATA = 0xC20C154,
		
		// Micro engine
		CP_ME_RAM_WADDR = 0xC20C15C,
		CP_ME_RAM_DATA = 0xC20C160
	};
	
	CPController(PhysicalMemory *physmem);
	
	void reset();
	void process();
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
	bool check_interrupts();
	
private:
	PhysicalMemory *physmem;
	
	PM4Processor pm4;
	
	uint32_t rb_base;
	uint32_t rb_cntl;
	uint32_t rb_rptr_addr;
	uint32_t rb_rptr;
	uint32_t rb_wptr;
	uint32_t rb_wptr_delay;
	uint32_t rb_rptr_wr;
	
	uint32_t pfp_ucode_addr;
	uint32_t pfp_ucode_data[0x350];
	
	uint32_t me_ram_addr;
	uint32_t me_ram_data[0x550];
};


class DMAProcessor {
public:
	DMAProcessor(PhysicalMemory *physmem);

	void reset();
	
	void process(uint32_t value);
	
	bool check_interrupts();
	
private:
	enum Opcode {
		DMA_PACKET_COPY = 3,
		DMA_PACKET_FENCE = 6,
		DMA_PACKET_TRAP = 7,
		DMA_PACKET_CONSTANT_FILL = 13,
		DMA_PACKET_NOP = 15
	};
	
	enum State {
		STATE_NEXT,
		STATE_ARGUMENTS
	};
	
	int get_arg_num(Opcode opcode);
	void execute();
	
	PhysicalMemory *physmem;
	
	State state;
	bool trap;
	
	uint32_t args[5];
	int argnum;
	int argidx;
};


class DMAController {
public:
	enum Register {
		DMA_RB_CNTL = 0xC20D000,
		DMA_RB_BASE = 0xC20D004,
		DMA_RB_RPTR = 0xC20D008,
		DMA_RB_WPTR = 0xC20D00C,
		DMA_RB_RPTR_ADDR_HI = 0xC20D01C,
		DMA_RB_RPTR_ADDR_LO = 0xC20D020,
		DMA_STATUS_REG = 0xC20D034
	};
	
	DMAController(PhysicalMemory *physmem);
	
	void reset();
	void process();
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
	bool check_interrupts();
	
private:
	PhysicalMemory *physmem;
	
	DMAProcessor processor;
	
	uint32_t rb_cntl;
	uint32_t rb_base;
	uint32_t rb_rptr;
	uint32_t rb_wptr;
	uint32_t rb_rptr_addr;
};


// HDP_MAP_BASE:
//   23 -  0: address >> 8
// 
// HDP_MAP_END:
//   23 -  0: address >> 8
// 
// HDP_INPUT_ADDR:
//   27 -  0: address >> 8
// 	 
// HDP_CONFIG:
//   16 -  7: bytes per pixel (log)
//    6 -  5: swap mode
//    4 -  0: tile mode << 1
// 
// HDP_DIMENSIONS:
//   29 -  8: image mask
//    7 -  0: row mask

class HDPHandle {
public:
	enum Register {
		HDP_MAP_BASE = 4,
		HDP_MAP_END = 8,
		HDP_INPUT_ADDR = 0xC,
		HDP_CONFIG = 0x10,
		HDP_DIMENSIONS = 0x14
	};
	
	void reset();
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
	bool check(uint32_t addr);
	
private:
	uint32_t map_base;
	uint32_t map_end;
	uint32_t input_addr;
	uint32_t config;
	uint32_t dimensions;
};


// Host data path
class HDPController {
public:
	enum Register {
		HDP_HOST_PATH_CNTL = 0xC202C00,
		HDP_NONSURFACE_BASE = 0xC202C04,
		HDP_NONSURFACE_INFO = 0xC202C08,
		HDP_NONSURFACE_SIZE = 0xC202C0C,
		
		HDP_HANDLE_START = 0xC202C10,
		HDP_HANDLE_END = 0xC202F10,
		
		HDP_TILING_START = 0xD0000000,
		HDP_TILING_END = 0xD2000000
	};
	
	void reset();
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
private:
	HDPHandle *find_handle(uint32_t addr);
	
	HDPHandle handles[32];
};


class GPUController {
public:
	enum Register {
		// Interrupt handling
		GPU_IH_RB_BASE = 0xC203E04,
		GPU_IH_RB_RPTR = 0xC203E08,
		GPU_IH_RB_WPTR_ADDR_LO = 0xC203E14,
		GPU_IH_STATUS = 0xC203E20,
		
		// Run list controller
		GPU_RLC_UCODE_ADDR = 0xC203F2C,
		GPU_RLC_UCODE_DATA = 0xC203F30,
		
		GPU_GRBM_SOFT_RESET = 0xC208020,
		
		GPU_WAIT_UNTIL = 0xC208040,
		
		GPU_SCRATCH_UMSK = 0xC208540,
		GPU_SCRATCH_ADDR = 0xC208544,
		
		GPU_SCRATCH_START = 0xC208500,
		GPU_SCRATCH_END = 0xC208520,
		
		GPU_GB_TILING_CONFIG = 0xC2098F0,
		GPU_CC_RB_BACKEND_DISABLE = 0xC2098F4,
		
		GPU_HDP_START = 0xC202C00,
		GPU_HDP_END = 0xC203000,
		
		GPU_DC0_START = 0xC206000,
		GPU_DC0_END = 0xC206800,
		
		GPU_DC1_START = 0xC206800,
		GPU_DC1_END = 0xC207000,
		
		GPU_CP_START = 0xC20C000,
		GPU_CP_END = 0xC20C800,
		
		GPU_DMA_START = 0xC20D000,
		GPU_DMA_END = 0xC20D800,
		
		GPU_TILING_START = 0xD0000000,
		GPU_TILING_END = 0xD2000000
	};
	
	GPUController(PhysicalMemory *physmem);
	
	void reset();
	void update();
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
	bool check_interrupts();
	
private:
	void trigger_irq(uint32_t type, uint32_t data1, uint32_t data2, uint32_t data3);

	PhysicalMemory *physmem;
	
	DCController dc0;
	DCController dc1;
	CPController cp;
	DMAController dma;
	HDPController hdp;
	
	uint32_t ih_rb_base;
	uint32_t ih_rb_rptr;
	uint32_t ih_rb_wptr_addr;

	uint32_t rlc_ucode_addr;
	uint32_t rlc_ucode_data[0x400];
	
	uint32_t scratch[8];
	uint32_t scratch_umsk;
	uint32_t scratch_addr;
	
	uint32_t gb_tiling_config;
	uint32_t cc_rb_backend_disable;
	
	uint32_t timer;
};
