
#include "hardware/gpu.h"
#include "physicalmemory.h"

#include "common/exceptions.h"
#include "common/endian.h"

#include <cstring>


void DCController::reset() {
	crtc_interrupt_control = 0;
	
	grph_enable = 0;
	grph_control = 0;
	grph_primary_surface_addr = 0;
	grph_secondary_surface_addr = 0;
	grph_dfq_status = 0;
	
	ovl_enable = 0;
	ovl_control1 = 0;
	ovl_surface_addr = 0;
	
	lut_autofill = 0;
}

uint32_t DCController::read(uint32_t addr) {
	switch (addr) {
		case D1CRTC_FORCE_COUNT_NOW_CNTL: return 0x10000;
		case D1CRTC_STATUS: return 1; //D1CRTC_V_BLANK
		case D1CRTC_INTERRUPT_CONTROL: return crtc_interrupt_control;
		
		case D1GRPH_ENABLE: return grph_enable;
		case D1GRPH_CONTROL: return grph_control;
		case D1GRPH_PRIMARY_SURFACE_ADDRESS: return grph_primary_surface_addr;
		case D1GRPH_SECONDARY_SURFACE_ADDRESS: return grph_secondary_surface_addr;
		case D1GRPH_DFQ_STATUS: return grph_dfq_status;
		
		case D1OVL_ENABLE: return ovl_enable;
		case D1OVL_CONTROL1: return ovl_control1;
		case D1OVL_SURFACE_ADDRESS: return ovl_surface_addr;
		
		case DC_LUT_AUTOFILL: return lut_autofill;
	}
	
	Logger::warning("Unknown dc read: 0x%X", addr);
	return 0;
}

void DCController::write(uint32_t addr, uint32_t value) {
	if (addr == D1CRTC_INTERRUPT_CONTROL) crtc_interrupt_control = value;
	
	else if (addr == D1GRPH_ENABLE) grph_enable = value;
	else if (addr == D1GRPH_CONTROL) grph_control = value;
	else if (addr == D1GRPH_PRIMARY_SURFACE_ADDRESS) grph_primary_surface_addr = value;
	else if (addr == D1GRPH_SECONDARY_SURFACE_ADDRESS) grph_secondary_surface_addr = value;
	else if (addr == D1GRPH_DFQ_CONTROL) {
		if (value & 1) {
			grph_dfq_status |= 0x100;
		}
	}
	else if (addr == D1GRPH_DFQ_STATUS) {
		if (value & 0x200) {
			grph_dfq_status &= ~0x100;
		}
	}
	
	else if (addr == D1OVL_ENABLE) ovl_enable = value;
	else if (addr == D1OVL_CONTROL1) ovl_control1 = value;
	else if (addr == D1OVL_SURFACE_ADDRESS) ovl_surface_addr = value;
	
	else if (addr == DC_LUT_PWL_DATA) {}
	else if (addr == DC_LUT_AUTOFILL) {
		if (value & 1) {
			lut_autofill = 2;
		}
	}
	
	else {
		Logger::warning("Unknown dc write: 0x%X (0x%08X)", addr, value);
	}
}


PM4Processor::PM4Processor(PhysicalMemory *physmem) {
	this->physmem = physmem;
	
	reset();
}

void PM4Processor::reset() {
	state = STATE_NEXT;
	args.clear();
	retired = false;
}

bool PM4Processor::check_interrupts() {
	bool ret = retired;
	retired = false;
	return ret;
}

void PM4Processor::process(uint32_t value) {
	if (state == STATE_NEXT) {
		int type = value >> 30;
		if (type == 0 || type == 3) {
			state = STATE_ARGUMENTS;
			argnum = ((value >> 16) & 0x3FFF) + 2;
			args.push_back(value);
		}
		else if (type == 2) {}
		else {
			Logger::warning("Unknown pm4 packet type: %i", type);
		}
	}
	else if (state == STATE_ARGUMENTS) {
		args.push_back(value);
		if (args.size() == argnum) {
			execute();
			args.clear();
			state = STATE_NEXT;
		}
	}
}

void PM4Processor::execute() {
	int type = args[0] >> 30;
	if (type == 0) {
		uint32_t addr = 0xC200000 | ((args[0] & 0xFFFF) << 2);
		for (int i = 1; i < args.size(); i++) {
			physmem->write<uint32_t>(addr, args[i]);
			addr += 4;
		}
	}
	else if (type == 3) {
		int opcode = (args[0] >> 8) & 0xFF;
		if (opcode == PM4_CONTEXT_CONTROL) {}
		else if (opcode == PM4_INDIRECT_BUFFER) {
			uint32_t addr = args.at(1) & ~3;
			uint32_t size = args.at(3);
			PM4Processor processor(physmem);
			for (uint32_t i = 0 ; i < size; i++) {
				uint32_t value = physmem->read<uint32_t>(addr + i * 4);
				processor.process(value);
			}
		}
		else if (opcode == PM4_MEM_WRITE) {
			uint32_t addr = args.at(1);
			uint32_t flags = args.at(2);
			uint32_t datalo = args.at(3);
			uint32_t datahi = args.at(4);
			if (flags & (1 << 18)) { // DATA32
				physmem->write<uint32_t>(addr & ~3, datalo);
			}
			else {
				physmem->write<uint64_t>(addr & ~7, datalo | ((uint64_t)datahi << 32));
			}
		}
		else if (opcode == PM4_ME_INITIALIZE) {} // Micro-engine initialization
		else if (opcode == PM4_EVENT_WRITE_EOP) {
			uint32_t addr = args.at(2);
			int datasel = args.at(3) >> 29;
			int intsel = (args.at(3) >> 24) & 3;
			uint32_t datalo = args.at(4);
			uint32_t datahi = args.at(5);
			if (intsel != 1) {
				if (datasel == 1) {
					physmem->write<uint32_t>(addr & ~3, datalo);
				}
				else if (datasel == 2) {
					addr &= ~7;
					physmem->write<uint32_t>(addr, datalo);
					physmem->write<uint32_t>(addr+4, datahi);
				}
			}
			if (intsel == 1 || intsel == 2) {
				retired = true;
			}
		}
		else if (opcode == PM4_SET_CONFIG_REG) {
			uint32_t addr = 0xC208000 + args.at(1) * 4;
			for (size_t i = 2; i < args.size(); i++) {
				physmem->write<uint32_t>(addr, args[i]);
				addr += 4;
			}
		}
		else if (opcode == PM4_SET_CONTEXT_REG) {
			uint32_t addr = 0xC228000 + args.at(1) * 4;
			for (size_t i = 2; i < args.size(); i++) {
				physmem->write<uint32_t>(addr, args[i]);
				addr += 4;
			}
		}
		else {
			Logger::warning("Unknown pm4 opcode: 0x%02X (%i dwords)", opcode, args.size());
		}
	}
	else {
		Logger::warning("Unknown pm4 packet type: %i", type);
	}
}


CPController::CPController(PhysicalMemory *physmem) :
	pm4(physmem)
{
	this->physmem = physmem;
}

bool CPController::check_interrupts() {
	return pm4.check_interrupts();
}

void CPController::reset() {
	pm4.reset();
	
	memset(pfp_ucode_data, 0, sizeof(pfp_ucode_data));
	memset(me_ram_data, 0, sizeof(me_ram_data));
	
	pfp_ucode_addr = 0;
	me_ram_addr = 0;
	
	rb_base = 0;
	rb_cntl = 0;
	rb_rptr_addr = 0;
	rb_rptr = 0;
	rb_wptr = 0;
	rb_wptr_delay = 0;
	rb_rptr_wr = 0;
}

void CPController::process() {
	uint32_t bufsize = 2 << (rb_cntl & 0x3F);
	
	while (rb_rptr != rb_wptr) {
		uint32_t value = physmem->read<uint32_t>(rb_base + rb_rptr * 4);
		pm4.process(value);
		rb_rptr = (rb_rptr + 1) % bufsize;
	}
	
	if (!(rb_cntl & 0x10)) {
		physmem->write<uint32_t>(rb_rptr_addr & ~3, rb_rptr);
	}
}

uint32_t CPController::read(uint32_t addr) {
	switch (addr) {
		case CP_RB_BASE: return rb_base >> 8;
		case CP_RB_CNTL: return rb_cntl;
		case CP_RB_RPTR_ADDR: return rb_rptr_addr;
		case CP_RB_RPTR: return rb_rptr;
		case CP_RB_WPTR: return rb_wptr;
		case CP_RB_WPTR_DELAY: return rb_wptr_delay;
		
		case CP_PFP_UCODE_DATA: {
			uint32_t result = pfp_ucode_data[pfp_ucode_addr++];
			pfp_ucode_addr %= 0x350;
			return result;
		}
		
		case CP_ME_RAM_DATA: {
			uint32_t result = me_ram_data[me_ram_addr++];
			me_ram_addr %= 0x550;
			return result;
		}
	}
	
	Logger::warning("Unknown cp read: 0x%X", addr);
	return 0;
}

void CPController::write(uint32_t addr, uint32_t value) {
	if (addr == CP_RB_BASE) rb_base = value << 8;
	else if (addr == CP_RB_CNTL) rb_cntl = value;
	else if (addr == CP_RB_RPTR_ADDR) rb_rptr_addr = value;
	else if (addr == CP_RB_WPTR) {
		rb_wptr = value;
		if (rb_cntl & 1) {
			rb_rptr = rb_rptr_wr;
		}
		process();
	}
	else if (addr == CP_RB_WPTR_DELAY) rb_wptr_delay = value;
	else if (addr == CP_RB_RPTR_WR) rb_rptr_wr = value;
	
	else if (addr == CP_PFP_UCODE_ADDR) pfp_ucode_addr = value % 0x350;
	else if (addr == CP_PFP_UCODE_DATA) {
		pfp_ucode_data[pfp_ucode_addr++] = value;
		pfp_ucode_addr %= 0x350;
	}
	
	else if (addr == CP_ME_RAM_WADDR) me_ram_addr = value % 0x550;
	else if (addr == CP_ME_RAM_DATA) {
		me_ram_data[me_ram_addr++] = value;
		me_ram_addr %= 0x550;
	}
	
	else {
		Logger::warning("Unknown cp write: 0x%X (0x%08X)", addr, value);
	}
}


DMAProcessor::DMAProcessor(PhysicalMemory *physmem) {
	this->physmem = physmem;
}

void DMAProcessor::reset() {
	state = STATE_NEXT;
	trap = false;
}

bool DMAProcessor::check_interrupts() {
	bool ints = trap;
	trap = false;
	return ints;
}

int DMAProcessor::get_arg_num(Opcode opcode) {
	switch (opcode) {
		case DMA_PACKET_COPY: return 5;
		case DMA_PACKET_FENCE: return 4;
		case DMA_PACKET_TRAP: return 1;
		case DMA_PACKET_CONSTANT_FILL: return 4;
		case DMA_PACKET_NOP: return 1;
	}
	runtime_error("Unknown dma opcode: %i", opcode);
	return 0;
}

void DMAProcessor::process(uint32_t value) {
	if (state == STATE_NEXT) {
		args[0] = value;
		argidx = 1;
		
		Opcode opcode = (Opcode)(value >> 28);
		argnum = get_arg_num(opcode);
		if (argnum > 1) {
			state = STATE_ARGUMENTS;
		}
		else {
			execute();
		}
	}
	else if (state == STATE_ARGUMENTS) {
		args[argidx++] = value;
		if (argidx == argnum) {
			execute();
		}
	}
}

void DMAProcessor::execute() {
	Opcode opcode = (Opcode)(args[0] >> 28);
	if (opcode == DMA_PACKET_COPY) {
		int dwords = args[0] & 0xFFFF;
		uint32_t dst = args[1];
		uint32_t src = args[2];
		for (int i = 0; i < dwords; i++) {
			uint32_t value = physmem->read<uint32_t>(src + i * 4);
			physmem->write<uint32_t>(dst + i * 4, value);
		}
	}
	else if (opcode == DMA_PACKET_FENCE) {
		uint32_t addr = args[1];
		uint32_t value = args[3];
		physmem->write<uint32_t>(addr, value);
	}
	else if (opcode == DMA_PACKET_TRAP) trap = true;
	else if (opcode == DMA_PACKET_CONSTANT_FILL) {
		int dwords = args[0] & 0xFFFF;
		uint32_t addr = args[1];
		uint32_t value = Endian::swap32(args[2]);
		for (int i = 0; i < dwords; i++) {
			physmem->write(addr + i * 4, value);
		}
	}
	else if (opcode == DMA_PACKET_NOP) {}
	else {
		runtime_error("Unknown dma opcode: %i", opcode);
	}
	
	state = STATE_NEXT;
}


DMAController::DMAController(PhysicalMemory *physmem) :
	processor(physmem)
{
	this->physmem = physmem;
}

void DMAController::reset() {
	processor.reset();
	
	rb_cntl = 0;
	rb_base = 0;
	rb_rptr = 0;
	rb_wptr = 0;
	rb_rptr_addr = 0;
}

bool DMAController::check_interrupts() {
	return processor.check_interrupts() && (rb_cntl & 1);
}

void DMAController::process() {
	if (rb_cntl & 1) {
		uint32_t size = 4 << ((rb_cntl >> 1) & 0x1F);
		while (rb_rptr != rb_wptr) {
			uint32_t value = physmem->read<uint32_t>(rb_base + rb_rptr);
			processor.process(value);
			rb_rptr = (rb_rptr + 4) % size;
		}
		
		if (rb_cntl & 0x1000) {
			physmem->write<uint32_t>(rb_rptr_addr, rb_rptr);
		}
	}
}

uint32_t DMAController::read(uint32_t addr) {
	switch (addr) {
		case DMA_RB_CNTL: return rb_cntl;
		case DMA_RB_BASE: return rb_base >> 8;
		case DMA_RB_RPTR: return rb_rptr;
		case DMA_RB_WPTR: return rb_wptr;
		case DMA_STATUS_REG: return 1; //idle
	}
	
	Logger::warning("Unknown dma read: 0x%X", addr);
	return 0;
}

void DMAController::write(uint32_t addr, uint32_t value) {
	if (addr == DMA_RB_CNTL) {
		rb_cntl = value;
		process();
	}
	else if (addr == DMA_RB_BASE) rb_base = value << 8;
	else if (addr == DMA_RB_RPTR) rb_rptr = value;
	else if (addr == DMA_RB_WPTR) {
		rb_wptr = value;
		process();
	}
	else if (addr == DMA_RB_RPTR_ADDR_HI) {}
	else if (addr == DMA_RB_RPTR_ADDR_LO) rb_rptr_addr = value;
	else {
		Logger::warning("Unknown dma write: 0x%X (0x%08X)", addr, value);
	}
}


void HDPHandle::reset() {}

uint32_t HDPHandle::read(uint32_t addr) {
	Logger::warning("Unknown hdp handle read: 0x%X", addr);
	return 0;
}

void HDPHandle::write(uint32_t addr, uint32_t value) {
	if (addr == HDP_MAP_BASE) map_base = value;
	else if (addr == HDP_MAP_END) map_end = value;
	else if (addr == HDP_INPUT_ADDR) input_addr = value;
	else if (addr == HDP_CONFIG) config = value;
	else if (addr == HDP_DIMENSIONS) dimensions = value;
	else {
		Logger::warning("Unknown hdp handle write: 0x%X (0x%08X)", addr, value);
	}
}

bool HDPHandle::check(uint32_t addr) {
	return map_base != map_end && addr >= map_base && addr <= map_end;
}


void HDPController::reset() {
	for (int i = 0; i < 32; i++) {
		handles[i].reset();
	}
}

uint32_t HDPController::read(uint32_t addr) {
	if (HDP_HANDLE_START <= addr && addr < HDP_HANDLE_END) {
		addr -= HDP_HANDLE_START;
		return handles[addr / 0x18].read(addr % 0x18);
	}
	else if (HDP_TILING_START <= addr && addr < HDP_TILING_END) {
		HDPHandle *handle = find_handle(addr - HDP_TILING_START);
		if (!handle) {
			return 0;
		}
		
		Logger::warning("Tiling aperture not implemented");
		return 0;
	}
	Logger::warning("Unknown hdp read: 0x%X", addr);
	return 0;
}

void HDPController::write(uint32_t addr, uint32_t value) {
	if (HDP_HANDLE_START <= addr && addr < HDP_HANDLE_END) {
		addr -= HDP_HANDLE_START;
		handles[addr / 0x18].write(addr % 0x18, value);
	}
	else {
		Logger::warning("Unknown hdp write: 0x%X (0x%08X)", addr, value);
	}
}

HDPHandle *HDPController::find_handle(uint32_t addr) {
	for (int i = 0; i < 32; i++) {
		if (handles[i].check(addr)) {
			return &handles[i];
		}
	}
	return nullptr;
}


GPUController::GPUController(PhysicalMemory *physmem) :
	cp(physmem),
	dma(physmem)
{
	this->physmem = physmem;
}

void GPUController::reset() {
	memset(rlc_ucode_data, 0, sizeof(rlc_ucode_data));
	memset(scratch, 0, sizeof(scratch));
	
	scratch_umsk = 0;
	scratch_addr = 0;
	
	ih_rb_base = 0;
	ih_rb_rptr = 0;
	ih_rb_wptr_addr = 0;
	
	rlc_ucode_addr = 0;
	
	gb_tiling_config = 0;
	cc_rb_backend_disable = 0;
	
	dc0.reset();
	dc1.reset();
	cp.reset();
	dma.reset();
	hdp.reset();
}

void GPUController::trigger_irq(uint32_t type, uint32_t data1, uint32_t data2, uint32_t data3) {
	uint32_t pos = physmem->read<uint32_t>(ih_rb_wptr_addr);
	physmem->write<uint32_t>(ih_rb_base + pos, type);
	physmem->write<uint32_t>(ih_rb_base + pos + 4, data1);
	physmem->write<uint32_t>(ih_rb_base + pos + 8, data2);
	physmem->write<uint32_t>(ih_rb_base + pos + 12, data3);
	physmem->write<uint32_t>(ih_rb_wptr_addr, pos + 16);
}

void GPUController::update() {
	timer++;
	if (timer == 35000) { // Trigger vsync
		if (dc0.crtc_interrupt_control & 0x01000000) trigger_irq(2, 3, 0, 0);
		if (dc1.crtc_interrupt_control & 0x01000000) trigger_irq(6, 3, 0, 0);
		timer = 0;
	}
	
	if (dma.check_interrupts()) {
		trigger_irq(0xE0, 0, 0, 0);
	}
	if (cp.check_interrupts()) {
		trigger_irq(0xB5, 0, 0, 0);
	}
}

bool GPUController::check_interrupts() {
	return ih_rb_wptr_addr && physmem->read<uint32_t>(ih_rb_wptr_addr) != ih_rb_rptr;
}

uint32_t GPUController::read(uint32_t addr) {
	switch (addr) {
		case GPU_IH_RB_BASE: return ih_rb_base >> 8;
		case GPU_IH_RB_RPTR: return ih_rb_rptr;
		case GPU_IH_RB_WPTR_ADDR_LO: return ih_rb_wptr_addr;
		case GPU_IH_STATUS: return 5;
		
		case GPU_RLC_UCODE_DATA: {
			uint32_t result = rlc_ucode_data[rlc_ucode_addr++];
			rlc_ucode_addr %= 0x400;
			return result;
		}
		
		case GPU_SCRATCH_UMSK: return scratch_umsk;
		case GPU_SCRATCH_ADDR: return scratch_addr >> 8;
		
		case GPU_GB_TILING_CONFIG: return gb_tiling_config;
		case GPU_CC_RB_BACKEND_DISABLE: return cc_rb_backend_disable;
	}
	
	if (GPU_SCRATCH_START <= addr && addr < GPU_SCRATCH_END) {
		uint32_t index = (addr - GPU_SCRATCH_START) / 4;
		return scratch[index];
	}
	
	if (GPU_DC0_START <= addr && addr < GPU_DC0_END) return dc0.read(addr - GPU_DC0_START);
	if (GPU_DC1_START <= addr && addr < GPU_DC1_END) return dc1.read(addr - GPU_DC1_START);
	if (GPU_CP_START <= addr && addr < GPU_CP_END) return cp.read(addr);
	if (GPU_DMA_START <= addr && addr < GPU_DMA_END) return dma.read(addr);
	if (GPU_HDP_START <= addr && addr < GPU_HDP_END) return hdp.read(addr);
	if (GPU_TILING_START <= addr && addr < GPU_TILING_END) return hdp.read(addr);
	
	Logger::warning("Unknown gpu read: 0x%X", addr);
	return 0;
}

void GPUController::write(uint32_t addr, uint32_t value) {
	if (addr == GPU_IH_RB_BASE) ih_rb_base = value << 8;
	else if (addr == GPU_IH_RB_RPTR) ih_rb_rptr = value;
	else if (addr == GPU_IH_RB_WPTR_ADDR_LO) ih_rb_wptr_addr = value;
	
	else if (addr == GPU_RLC_UCODE_ADDR) rlc_ucode_addr = value % 0x400;
	else if (addr == GPU_RLC_UCODE_DATA) {
		rlc_ucode_data[rlc_ucode_addr++] = value;
		rlc_ucode_addr %= 0x400;
	}
	
	else if (addr == GPU_GRBM_SOFT_RESET) {
		if (value & 1) {
			cp.reset();
		}
	}
	
	else if (addr == GPU_WAIT_UNTIL) {}
	
	else if (addr == GPU_SCRATCH_UMSK) scratch_umsk = value;
	else if (addr == GPU_SCRATCH_ADDR) scratch_addr = value << 8;
	
	else if (addr == GPU_GB_TILING_CONFIG) gb_tiling_config = value;
	else if (addr == GPU_CC_RB_BACKEND_DISABLE) cc_rb_backend_disable = value;
	
	else if (GPU_SCRATCH_START <= addr && addr < GPU_SCRATCH_END) {
		uint32_t index = (addr - GPU_SCRATCH_START) / 4;
		scratch[index] = value;
		if (scratch_umsk & (1 << index)) {
			physmem->write<uint32_t>(scratch_addr + index * 4, value);
		}
	}
	
	else if (GPU_DC0_START <= addr && addr < GPU_DC0_END) dc0.write(addr - GPU_DC0_START, value);
	else if (GPU_DC1_START <= addr && addr < GPU_DC1_END) dc1.write(addr - GPU_DC1_START, value);
	else if (GPU_CP_START <= addr && addr < GPU_CP_END) cp.write(addr, value);
	else if (GPU_DMA_START <= addr && addr < GPU_DMA_END) dma.write(addr, value);
	else if (GPU_HDP_START <= addr && addr < GPU_HDP_END) hdp.write(addr, value);
	else if (GPU_TILING_START <= addr && addr < GPU_TILING_END) hdp.write(addr, value);
	
	else {
		Logger::warning("Unknown gpu write: 0x%X (0x%08X)", addr, value);
	}
}
