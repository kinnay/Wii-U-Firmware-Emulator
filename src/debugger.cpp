
#include "emulator.h"
#include "debugger.h"
#include "expression.h"
#include "strings.h"

#include "common/sys.h"
#include "common/buffer.h"
#include "common/exceptions.h"

#include <algorithm>


const char *HELP_TEXT =
	"General:\n"
	"    help\n"
	"    exit\n"
	"    quit\n"
	"    select arm/ppc0/ppc1/ppc2\n"
	"\n"
	"Emulation:\n"
	"    step (<steps>)\n"
	"    run\n"
	"    reset\n"
	"    restart\n"
	#if STATS
	"    stats\n"
	#endif
	#if METRICS
	"    metrics ppc0/ppc1/ppc2 category/frequency\n"
	"    syscalls ppc0/ppc1/ppc2\n"
	#endif
	#if BREAKPOINTS || WATCHPOINTS
	"\n"
	"Debugging:\n"
	#endif
	#if BREAKPOINTS
	"    break add/del <address>\n"
	"    break list\n"
	"    break clear\n"
	#endif
	#if BREAKPOINTS && WATCHPOINTS
	"    \n"
	#endif
	#if WATCHPOINTS
	"    watch add/del virt/phys read/write <address>\n"
	"    watch list\n"
	"    watch clear\n"
	#endif
	"\n"
	"CPU state:\n"
	"    state (full)\n"
	"    print <expr>\n"
	"    trace\n"
	"\n"
	"Memory state:\n"
	"    read virt/phys <address> <length>\n"
	"    translate <address>\n"
	"    memmap\n"
	"\n"
	"System state:\n"
	"    modules\n"
	"    module <name>\n"
	"    threads\n"
	"    thread <id>\n"
	"    queues\n"
	"    queue <id>\n"
	"    devices\n"
	"    hardware\n"
	"\n"
	"Filesystem state:\n"
	"    volumes\n"
	"    fileclients\n"
	"    slccache\n"
;


static uint64_t divide(uint64_t a, uint64_t b) {
	return b ? a / b : 0;
}

static int percentage(uint64_t value, uint64_t total) {
	return divide(value * 100, total);
}


ArgParser::ArgParser(Ref<EvalContext> context, const std::vector<std::string> &args) {
	this->context = context;
	this->args = args;
	offset = 1;
}

bool ArgParser::eof() {
	return offset >= args.size();
}

bool ArgParser::finish() {
	if (!eof()) {
		Sys::stdout->write("Too many arguments.\n");
		return false;
	}
	return true;
}

bool ArgParser::check() {
	if (eof()) {
		Sys::stdout->write("Not enough arguments.\n");
		return false;
	}
	return true;
}

bool ArgParser::string(std::string *value) {
	if (!check()) return false;
	*value = args[offset++];
	return true;
}

bool ArgParser::integer(uint32_t *value) {
	if (!check()) return false;
	
	ExpressionParser parser;
	Ref<Node> expr = parser.parse(args[offset++]);
	if (!expr || !expr->evaluate(context, value)) {
		Sys::stdout->write("Failed to parse argument.\n");
		return false;
	}
	return true;
}


const char *AccessProtNames[] = {
	" no access", " read only", "read/write"
};


MemoryRegion::MemoryRegion() {
	virt = 0;
	phys = 0;
	size = 0;
	user = NoAccess;
	system = NoAccess;
	executable = true;
}

bool MemoryRegion::check() {
	if (size == 0) return false;
	if (virt + size > 0x100000000) return false;
	if (phys + size > 0x100000000) return false;
	return true;
}

bool MemoryRegion::overlaps(Ref<MemoryRegion> other) {
	return virt < other->virt + other->size && virt + size > other->virt;
}

bool MemoryRegion::hits(Ref<MemoryRegion> other) {
	if (user != other->user) return false;
	if (system != other->system) return false;
	return virt + size == other->virt || virt == other->virt + other->size;
}

void MemoryRegion::merge(Ref<MemoryRegion> other) {
	if (other->virt < virt) {
		virt = other->virt;
	}
	size += other->size;
}

bool MemoryRegion::compare(Ref<MemoryRegion> a, Ref<MemoryRegion> b) {
	return a->virt < b->virt;
}


MemoryMap::MemoryMap(bool nx) {
	this->nx = nx;
}

Ref<MemoryRegion> MemoryMap::find(Ref<MemoryRegion> region) {
	for (Ref<MemoryRegion> other : regions) {
		if (other->overlaps(region)) {
			runtime_error("Memory regions overlap");
		}
		if (other->hits(region)) {
			return other;
		}
	}
	return nullptr;
}

void MemoryMap::add(Ref<MemoryRegion> region) {
	if (!region->check()) runtime_error("Memory region is invalid");
	
	Ref<MemoryRegion> prev = find(region);
	if (prev) {
		prev->merge(region);
	}
	else {
		regions.push_back(region);
	}
}

void MemoryMap::print() {
	if (regions.size() == 0) {
		Sys::stdout->write("    no pages mapped\n");
		return;
	}
	
	std::sort(regions.begin(), regions.end(), MemoryRegion::compare);
	
	for (Ref<MemoryRegion> region : regions) {
		Sys::stdout->write(
			"    %08X-%08X => %08X-%08X (user: %s, system: %s",
			region->virt, region->virt + region->size,
			region->phys, region->phys + region->size,
			AccessProtNames[region->user],
			AccessProtNames[region->system]
		);
		
		if (nx) {
			Sys::stdout->write(", %s", region->executable ? "executable" : "no execute");
		}
		Sys::stdout->write(")\n");
	}
}


const char *getModeName(ARMCore::ProcessorMode mode) {
	switch (mode) {
		case ARMCore::User: return "USER";
		case ARMCore::FIQ: return "FIQ";
		case ARMCore::IRQ: return "IRQ";
		case ARMCore::SVC: return "SVC";
		case ARMCore::Abort: return "ABORT";
		case ARMCore::Undefined: return "UNDEFINED";
		case ARMCore::System: return "SYSTEM";
	}
	return "<invalid>";
}

ARMDebugger::ARMDebugger(PhysicalMemory *physmem, ARMProcessor *cpu) {
	this->physmem = physmem;
	this->cpu = cpu;
}

Processor *ARMDebugger::getProcessor() {
	return cpu;
}

std::string ARMDebugger::name() {
	return "ARM";
}

Ref<EvalContext> ARMDebugger::getContext() {
	Ref<EvalContext> context = new EvalContext();
	
	for (int i = 0; i < 16; i++) {
		std::string reg = StringUtils::format("R%i", i);
		context->add(reg, cpu->core.regs[i]);
	}
	context->add("SP", cpu->core.regs[13]);
	context->add("LR", cpu->core.regs[14]);
	context->add("PC", cpu->core.regs[15]);
	
	context->add("CPSR", cpu->core.cpsr);
	context->add("SPSR", cpu->core.spsr);
	
	context->add("DFSR", cpu->core.dfsr);
	context->add("IFSR", cpu->core.ifsr);
	context->add("FAR", cpu->core.dfsr);
	
	return context;
}

uint32_t ARMDebugger::pc() {
	return cpu->core.regs[ARMCore::PC];
}

void ARMDebugger::step() {
	cpu->step();
}

bool ARMDebugger::translate(uint32_t *address) {
	return cpu->mmu.translate(address, MemoryAccess::DataRead, true);
}

void ARMDebugger::printState() {
	Sys::stdout->write(
		"R0 = %08X R1 = %08X R2 = %08X R3 = %08X R4 = %08X\n"
		"R5 = %08X R6 = %08X R7 = %08X R8 = %08X R9 = %08X\n"
		"R10= %08X R11= %08X R12= %08X\n"
		"SP = %08X LR = %08X PC = %08X\n\n"
		"CPSR = %08X SPSR = %08X\n",
		cpu->core.regs[0], cpu->core.regs[1], cpu->core.regs[2],
		cpu->core.regs[3], cpu->core.regs[4], cpu->core.regs[5],
		cpu->core.regs[6], cpu->core.regs[7], cpu->core.regs[8],
		cpu->core.regs[9], cpu->core.regs[10], cpu->core.regs[11],
		cpu->core.regs[12], cpu->core.regs[13], cpu->core.regs[14],
		cpu->core.regs[15], cpu->core.cpsr, cpu->core.spsr
	);
}

void ARMDebugger::printStateDetailed() {
	cpu->core.writeModeRegs();
	
	Sys::stdout->write("Current mode: %s\n\n", getModeName(cpu->core.getMode()));
	Sys::stdout->write(
		"User mode:\n"
		"    R0 = %08X R1 = %08X R2 = %08X R3 = %08X\n"
		"    R4 = %08X R5 = %08X R6 = %08X R7 = %08X\n"
		"    R8 = %08X R9 = %08X R10= %08X R11= %08X\n"
		"    R12= %08X SP = %08X LR = %08X PC = %08X\n"
		"    CPSR = %08X SPSR = %08X\n",
		cpu->core.regsUser[0], cpu->core.regsUser[1], cpu->core.regsUser[2],
		cpu->core.regsUser[3], cpu->core.regsUser[4], cpu->core.regsUser[5],
		cpu->core.regsUser[6], cpu->core.regsUser[7], cpu->core.regsUser[8],
		cpu->core.regsUser[9], cpu->core.regsUser[10], cpu->core.regsUser[11],
		cpu->core.regsUser[12], cpu->core.regsUser[13], cpu->core.regsUser[14],
		cpu->core.regsUser[15], cpu->core.cpsr, cpu->core.spsr
	);
	Sys::stdout->write(
		"FIQ mode:\n"
		"    R8 = %08X R9 = %08X R10= %08X R11 = %08X\n"
		"    R12= %08X SP = %08X LR = %08X SPSR= %08X\n",
		cpu->core.regsFiq[0], cpu->core.regsFiq[1], cpu->core.regsFiq[2],
		cpu->core.regsFiq[3], cpu->core.regsFiq[4], cpu->core.regsFiq[5],
		cpu->core.regsFiq[6], cpu->core.spsrFiq
	);
	Sys::stdout->write(
		"IRQ mode:\n"
		"    SP = %08X LR = %08X SPSR = %08X\n",
		cpu->core.regsIrq[0], cpu->core.regsIrq[1], cpu->core.spsrIrq
	);
	Sys::stdout->write(
		"SVC mode:\n"
		"    SP = %08X LR = %08X SPSR = %08X\n",
		cpu->core.regsSvc[0], cpu->core.regsSvc[1], cpu->core.spsrSvc
	);
	Sys::stdout->write(
		"Abort mode:\n"
		"    SP = %08X LR = %08X SPSR = %08X\n",
		cpu->core.regsAbt[0], cpu->core.regsAbt[1], cpu->core.spsrAbt
	);
	Sys::stdout->write(
		"Undefined mode:\n"
		"    SP = %08X LR = %08X SPSR = %08X\n\n",
		cpu->core.regsUnd[0], cpu->core.regsUnd[1], cpu->core.spsrUnd
	);
	Sys::stdout->write(
		"Other registers:\n"
		"    TTBR = %08X FAR  = %08X\n"
		"    DFSR = %08X IFSR = %08X\n",
		cpu->core.ttbr, cpu->core.far,
		cpu->core.dfsr, cpu->core.ifsr
	);
}

void ARMDebugger::printStackTrace() {
	uint32_t sp = cpu->core.regs[ARMCore::SP];
	uint32_t lr = cpu->core.regs[ARMCore::LR];
	uint32_t pc = cpu->core.regs[ARMCore::PC];
	bool thumb = cpu->core.isThumb();
	
	Sys::stdout->write("PC = 0x%08X  LR = 0x%08X  SP = 0x%08X\n", pc, lr, sp);
	
	translate(&sp);
	translate(&pc);
	
	if (thumb) {
		uint16_t instr = physmem->read<uint16_t>(pc);
		if ((instr & ~0xF0) == 0xB500) {
			uint16_t prev = physmem->read<uint16_t>(pc - 2);
			if ((prev & ~0x7F) == 0xB080) { // SUB
				sp += (prev & 0x7F) * 4;
			}
			
			pc = lr & ~1;
			thumb = lr & 1;
		}
	}
	else {
		uint32_t instr = physmem->read<uint32_t>(pc);
		if (instr == 0xE52DE004) {
			translate(&lr);
			pc = lr & ~1;
			thumb = lr & 1;
		}
		else if ((instr & ~0x1FF0) == 0xE92D4000) { // STMFD
			translate(&lr);
			pc = lr & ~1;
			thumb = lr & 1;
		}
	}
	
	Sys::stdout->write("Stack trace:\n");
	while (true) {
		if (pc == 0xD400200) break;
		
		if (thumb) {
			pc -= 2;
			
			uint16_t instr = physmem->read<uint16_t>(pc);
			if (instr == 0) {
				Sys::stdout->write("Stack trace may be unreliable.\n");
				break;
			}
			
			if ((instr & ~0x7F) == 0xB080) { // SUB
				sp += (instr & 0x7F) * 4;
			}
			else if ((instr & ~0xF0) == 0xB500) { // PUSH
				for (int i = 4; i < 8; i++) {
					if (instr & (1 << i)) {
						sp += 4;
					}
				}
				
				uint16_t prev = physmem->read<uint16_t>(pc - 2);
				
				pc = physmem->read<uint32_t>(sp);
				sp += 4;
				
				if ((prev & ~0x7F) == 0xB080) { // SUB
					sp += (prev & 0x7F) * 4;
				}
				
				if (pc & 1) {
					pc &= ~1;
					Sys::stdout->write("    0x%08X (thumb)\n", pc);
				}
				else {
					thumb = false;
					Sys::stdout->write("    0x%08X (arm)\n", pc);
				}
				
				translate(&pc);
			}
		}
		else {
			pc -= 4;
			
			uint32_t instrs[4];
			for (int i = 0; i < 4; i++) {
				instrs[i] = physmem->read<uint32_t>(pc + i * 4);
			}
			
			if (instrs[1] == 0xE1A01000 &&
				instrs[2] == 0xE3A00000 &&
				instrs[3] == 0xE7F002F0) break;
			
			uint32_t instr = instrs[0];
			if (instr == 0) {
				Sys::stdout->write("Stack trace may be unreliable.\n");
				break;
			}
			
			if (instr == 0xE52DE004) { // STR LR, [SP,#-4]!
				pc = physmem->read<uint32_t>(sp);
				sp += 4;
				
				if (pc & 1) {
					thumb = true;
					pc &= ~1;
					Sys::stdout->write("    0x%08X (thumb)\n", pc);
				}
				else {
					Sys::stdout->write("    0x%08X (arm)\n", pc);
				}
				
				translate(&pc);
			}
			else if ((instr & ~0x1FF0) == 0xE92D4000) { // STMFD
				for (int i = 4; i <= 12; i++) {
					if (instr & (1 << i)) {
						sp += 4;
					}
				}
				
				pc = physmem->read<uint32_t>(sp);
				sp += 4;
				
				if (pc & 1) {
					thumb = true;
					pc &= ~1;
					Sys::stdout->write("    0x%08X (thumb)\n", pc);
				}
				else {
					Sys::stdout->write("    0x%08X (arm)\n", pc);
				}
				
				translate(&pc);
			}
			else if ((instr & ~0xFFF) == 0xE24DD000) { // SUB
				uint32_t rot = ((instr >> 8) & 0xF) * 2;
				uint32_t imm = instr & 0xFF;
				
				if (rot) {
					imm = (imm >> rot) | (imm << (32 - rot));
				}
				
				sp += imm;
			}
		}
	}
}

MemoryRegion::Access AccessProtUser[] = {
	MemoryRegion::NoAccess, MemoryRegion::NoAccess,
	MemoryRegion::ReadOnly, MemoryRegion::ReadWrite
};
MemoryRegion::Access AccessProtSystem[] = {
	MemoryRegion::NoAccess, MemoryRegion::ReadWrite,
	MemoryRegion::ReadWrite, MemoryRegion::ReadWrite
};

void ARMDebugger::printMemoryMap() {
	if (!(cpu->core.control & 1)) {
		Sys::stdout->write("MMU is disabled\n");
		return;
	}
	
	MemoryMap memory(false);
	
	uint32_t ttbr = cpu->core.ttbr;
	for (uint32_t i = 0; i <= 0xFFF; i++) {
		uint32_t firstLevelDesc = physmem->read<uint32_t>(ttbr + i * 4);
		int firstLevelType = firstLevelDesc & 3;
		if (firstLevelType == 0) {}
		else if (firstLevelType == 1) {
			uint32_t secondLevelBase = firstLevelDesc & ~0x3FF;
			for (uint32_t j = 0; j <= 0xFF; j++) {
				uint32_t secondLevelDesc = physmem->read<uint32_t>(secondLevelBase + j * 4);
				int secondLevelType = secondLevelDesc & 3;
				if (secondLevelType == 0) {}
				else if (secondLevelType == 2) {
					for (int page = 0; page < 4; page++) {
						int ap = (secondLevelDesc >> (4 + page * 2)) & 3;
						
						Ref<MemoryRegion> region = new MemoryRegion();
						region->virt = (i << 20) + (j << 12) + page * 0x400;
						region->phys = (secondLevelDesc & ~0xFFF) + page * 0x400;
						region->size = 0x400;
						region->user = AccessProtUser[ap];
						region->system = AccessProtSystem[ap];
						memory.add(region);
					}
				}
				else {
					Logger::warning("Unsupported second-level descriptor type: %i\n", secondLevelType);
				}
			}
		}
		else if (firstLevelType == 2) {
			int ap = (firstLevelDesc >> 10) & 3;
			
			Ref<MemoryRegion> region = new MemoryRegion();
			region->virt = i << 20;
			region->phys = firstLevelDesc & ~0xFFFFF;
			region->size = 0x100000;
			region->user = AccessProtUser[ap];
			region->system = AccessProtSystem[ap];
			memory.add(region);
		}
		else {
			Logger::warning("Unsupported first-level descriptor type: %i\n", firstLevelType);
		}
	}
	
	memory.print();
}

const char *StateNames[] = {
	"free", "ready", "running", "inactive",
	"waiting", "canceled", "crashed"
};

const char *DeviceNames[] = {
	"ALARM", "NAND", "AES", "SHA", "EHCI-0", "OHCI-0:0",
	"OHCI-0:1", "SD CARD", "802.11", "GPIO", "SYSPROT", "11",
	"12", "13", "14", "POWER", "DRIVE", "17", "RTC",
	"19", "20", "21", "22", "23", "24", "25", "SATA", "27",
	"28", "IPC (Compat)", "MLC", "SDIO", "32", "OHCI-1:0",
	"EHCI-2", "OHCI-2:0", "36", "37", "AESS", "SHAS", "40",
	"41", "42", "I2C (PPC)", "I2C (ARM)", "IPC (CPU2)",
	"IPC (CPU1)", "IPC (CPU0)"
};

std::string ARMDebugger::getQueueSuffix(uint32_t id) {
	uint32_t queue = 0x8150C84 + (id & 0xFFF) * 0x20;
	uint8_t flags = physmem->read<uint8_t>(queue + 0x1D);
	if (flags & 1) {
		for (int i = 0; i < 48; i++) {
			uint32_t event = 0x81A3800 + i * 0x10;
			uint32_t handler = physmem->read<uint32_t>(event);
			uint32_t message = physmem->read<uint32_t>(event + 4);
			if (handler == queue) {
				return StringUtils::format(" [%s interrupts]", DeviceNames[i]);
			}
		}
	}
	for (int i = 0; i < 96; i++) {
		uint32_t device = 0x8166C00 + 0xC + i * 0x40;
		uint32_t queueId = physmem->read<uint32_t>(device + 0x20);
		if (queueId == id) {
			return StringUtils::format(" server for %s", physmem->read<std::string>(device));
		}
	}
	return "";
}

void ARMDebugger::printThreads() {
	uint32_t thread = 0xFFFF4D78;
	for (int i = 0; i < 180; i++) {
		uint32_t state = physmem->read<uint32_t>(thread + 0x50);
		if (state != 0 && state <= 6) {
			uint32_t pid = physmem->read<uint32_t>(thread + 0x54);
			uint32_t tid = physmem->read<uint32_t>(thread + 0x58);
			
			uint32_t regs[16];
			uint32_t cpsr = physmem->read<uint32_t>(thread + 0x6C);
			if (cpsr) {
				for (int i = 0; i < 16; i++) {
					regs[i] = physmem->read<uint32_t>(thread + i * 4 + 0x70);
				}
			}
			else {
				cpsr = physmem->read<uint32_t>(thread);
				for (int i = 0; i < 16; i++) {
					regs[i] = physmem->read<uint32_t>(thread + i * 4 + 4);
				}
			}
			
			std::string info;
			
			if (state == 4 && (cpu->core.control & 1)) {
				uint32_t addr = regs[15] - 4;
				translate(&addr);
				
				uint32_t instr = physmem->read<uint32_t>(addr);
				if ((instr & ~0xFF00) == 0xE7F000F0) {
					uint32_t syscall = (instr >> 8) & 0xFF;
					if (syscall == 1) {
						info = StringUtils::format("IOS_JoinThread (%i)", regs[0]);
					}
					else if (syscall == 8) {
						info = StringUtils::format("IOS_SuspendThread (%i)", regs[0]);
					}
					else if (syscall == 16) {
						info = StringUtils::format("IOS_ReceiveMessage (%i)", regs[0] & 0xFFF);
						info += getQueueSuffix(regs[0]);
					}
					else if (syscall == 51) {
						uint32_t message = regs[0];
						translate(&message);
						
						info = StringUtils::format("IOS_Open (%s)", physmem->read<std::string>(message));
					}
					else if (syscall == 52) {
						uint32_t array = 0x816840C + pid * 0x728;
						uint32_t entry = array + 0x1C + (regs[0] & 0xFFF) * 0x10;
						uint32_t device = physmem->read<uint32_t>(entry + 8);
						std::string name = physmem->read<std::string>(device);
						
						info = StringUtils::format("IOS_Close (%s)", name);
					}
					else if (syscall == 56) {
						uint32_t array = 0x816840C + pid * 0x728;
						uint32_t entry = array + 0x1C + (regs[0] & 0xFFF) * 0x10;
						uint32_t device = physmem->read<uint32_t>(entry + 8);
						std::string name = physmem->read<std::string>(device);
						
						info = StringUtils::format("IOS_Ioctl (%s, %i)", name, regs[1]);
					}
					else if (syscall == 57) {
						uint32_t array = 0x816840C + pid * 0x728;
						uint32_t entry = array + 0x1C + (regs[0] & 0xFFF) * 0x10;
						uint32_t device = physmem->read<uint32_t>(entry + 8);
						std::string name = physmem->read<std::string>(device);
						
						info = StringUtils::format("IOS_Ioctlv (%s, %i)", name, regs[1]);
					}
					else if (syscall == 88) {
						info = StringUtils::format("IOS_WaitSemaphore (%i)", regs[0] & 0xFFF);
					}
					else {
						info = StringUtils::format("Syscall (%i)", syscall);
					}
				}
			}
			
			if (pid < sizeof(ProcessNames) / sizeof(ProcessNames[0])) {
				Sys::stdout->write(
					" %3i:  %-12s PC=%08X  LR=%08X  (%s)  %s\n", tid,
					ProcessNames[pid], regs[15], regs[14],
					StateNames[state], info
				);
			}
		}
		thread += 0xC8;
	}
}

void ARMDebugger::printThreadDetails(uint32_t id) {
	if (id >= 180) {
		Sys::stdout->write("Invalid thread id.\n");
		return;
	}
	
	uint32_t thread = 0xFFFF4D78 + id * 0xC8;
	uint32_t state = physmem->read<uint32_t>(thread + 0x50);
	
	if (state == 0) {
		Sys::stdout->write("Thread is not in use.\n");
		return;
	}
	
	if (state > 6) {
		Sys::stdout->write("Thread has invalid state.\n");
		return;
	}
	
	uint32_t regs[16];
	uint32_t cpsr = physmem->read<uint32_t>(thread + 0x6C);
	if (cpsr) {
		for (int i = 0; i < 16; i++) {
			regs[i] = physmem->read<uint32_t>(thread + i * 4 + 0x70);
		}
	}
	else {
		cpsr = physmem->read<uint32_t>(thread);
		for (int i = 0; i < 16; i++) {
			regs[i] = physmem->read<uint32_t>(thread + i * 4 + 4);
		}
	}
	
	uint32_t pid = physmem->read<uint32_t>(thread + 0x54);
	
	if (pid >= sizeof(ProcessNames) / sizeof(ProcessNames[0])) {
		Sys::stdout->write("Thread has invalid pid.\n");
		return;
	}
	
	Sys::stdout->write("Thread is owned by %s\n\n", ProcessNames[pid]);
	
	uint32_t priority = physmem->read<uint32_t>(thread + 0x4C);
	Sys::stdout->write("Priority: %i\n\n", priority);
	
	Sys::stdout->write(
		"Registers:\n"
		"    R0 = %08X R1 = %08X R2 = %08X R3 = %08X\n"
		"    R4 = %08X R5 = %08X R6 = %08X R7 = %08X\n"
		"    R8 = %08X R9 = %08X R10= %08X R11= %08X\n"
		"    R12= %08X SP = %08X LR = %08X PC = %08X\n"
		"    CPSR = %08X\n\n",
		regs[0], regs[1], regs[2], regs[3], regs[4],
		regs[5], regs[6], regs[7], regs[8], regs[9],
		regs[10], regs[11], regs[12], regs[13],
		regs[14], regs[15], cpsr
	);
	
	uint32_t stackTop = physmem->read<uint32_t>(thread + 0xB4);
	uint32_t stackSize = physmem->read<uint32_t>(thread + 0xB8);
	Sys::stdout->write("Stack: 0x%08X - 0x%08X (0x%X bytes)\n\n", stackTop - stackSize, stackTop, stackSize);
	
	if (state == 1) Sys::stdout->write("This thread is ready to run.\n");
	else if (state == 2) Sys::stdout->write("This thread is running right now.\n");
	else if (state == 3) Sys::stdout->write("This thread is suspended.\n");
	else if (state == 4) Sys::stdout->write("This thread is currently waiting.\n");
	else if (state == 5) Sys::stdout->write("This thread has been canceled.\n");
	else if (state == 6) Sys::stdout->write("This thread has triggered an exception.\n");
}

void ARMDebugger::printMessageQueues() {
	uint32_t queue = 0x8150C84;
	for (int i = 0; i < 750; i++) {
		uint32_t capacity = physmem->read<uint32_t>(queue + 0x10);
		uint32_t buffer = physmem->read<uint32_t>(queue + 0x14);
		uint8_t pid = physmem->read<uint8_t>(queue + 0x1C);
		if (capacity != 0 && pid < sizeof(ProcessNames) / sizeof(ProcessNames[0])) {
			Sys::stdout->write(
				" %3i:  %-12s  Buffer: %08X  Capacity: %i\n",
				i, ProcessNames[pid], buffer, capacity
			);
		}
		queue += 0x20;
	}
}

void ARMDebugger::printMessageQueueDetails(uint32_t id) {
	uint32_t queue = 0x8150C84 + id * 0x20;
	
	uint32_t capacity = physmem->read<uint32_t>(queue + 0x10);
	uint32_t buffer = physmem->read<uint32_t>(queue + 0x14);
	uint8_t pid = physmem->read<uint8_t>(queue + 0x1C);
	uint8_t use = physmem->read<uint8_t>(queue + 0x1D);
	
	if (capacity == 0) {
		Sys::stdout->write("Message queue is not in use.\n");
		return;
	}
	
	if (pid >= sizeof(ProcessNames) / sizeof(ProcessNames[0])) {
		Sys::stdout->write("Message queue has invalid pid.\n");
		return;
	}
	
	Sys::stdout->write(
		"Owner: %-12s Buffer: %08X  Capacity: %i\n",
		ProcessNames[pid], buffer, capacity
	);
	
	if (use & 1) {
		for (int i = 0; i < 48; i++) {
			uint32_t event = 0x81A3800 + i * 0x10;
			uint32_t handler = physmem->read<uint32_t>(event);
			uint32_t message = physmem->read<uint32_t>(event + 4);
			if (handler == queue) {
				Sys::stdout->write(
					"Message queue is registered as interrupt handler for %s.\n",
					DeviceNames[i]
				);
				break;
			}
		}
	}
}

const char *ResourceStates[] = {
	"INVALID", "REGISTERED", "NOTREGISTERED",
	"RESUMED", "SUSPENDED", "PENDING", "FAILED"
};

void ARMDebugger::printDevices() {
	uint32_t device = 0x50711A8;
	if (cpu->core.control & 1) {
		translate(&device);
		
		for (int i = 0; i < 86; i++) {
			uint32_t name = physmem->read<uint32_t>(device);
			uint32_t error = physmem->read<uint32_t>(device + 0xC);
			uint32_t state = physmem->read<uint32_t>(device + 0x18);
			if (name && state < 7) {
				translate(&name);
				Sys::stdout->write(
					" %-24s %-16s (0x%X)\n", physmem->read<std::string>(name),
					ResourceStates[state], error
				);
			}
			device += 0x58;
		}
	}
}

void ARMDebugger::printFileClients() {
	uint32_t client = 0x111AB2A4;
	if (cpu->core.control & 1) {
		translate(&client);
		for (int i = 0; i < 0x270; i++) {
			uint32_t valid = physmem->read<uint32_t>(client);
			if (valid == 1) {
				std::string device = physmem->read<std::string>(client + 0x20);
				std::string cwd = physmem->read<std::string>(client + 0xAC);
				Sys::stdout->write(" %3i:  %-8s  %s\n", i, device, cwd);
			}
			client += 0x330;
		}
	}
}

void ARMDebugger::printVolumes() {
	if (cpu->core.control & 1) {
		uint32_t volume = 0x113FC280;
		translate(&volume);
		
		int index = 0;
		
		volume = physmem->read<uint32_t>(volume);
		while (volume) {
			translate(&volume);
			
			uint32_t fs = physmem->read<uint32_t>(volume + 0x5C);
			translate(&fs);
			
			std::string name = physmem->read<std::string>(volume + 0x1C8);
			std::string format = physmem->read<std::string>(fs + 8);
			
			Sys::stdout->write(" %2i:  %-10s %s\n", index, name, format);
			
			volume = physmem->read<uint32_t>(volume);
			
			index++;
		}
	}
}

void ARMDebugger::printSlcCacheState() {
	if (cpu->core.control & 1) {
		uint32_t numPtr = 0x1108B8A8;
		translate(&numPtr);
		
		uint32_t num = physmem->read<uint32_t>(numPtr);
		if (num == 0) {
			Sys::stdout->write("No cached areas found.\n");
		}
		else {
			uint32_t indexPtr = 0x1108B8A4;
			translate(&indexPtr);
			
			uint32_t index = physmem->read<uint32_t>(indexPtr);
			for (int i = 0; i < num; i++) {
				uint32_t addr = 0x110A1880 + index * 0x30;
				translate(&addr);
				
				uint64_t startBlock = physmem->read<uint64_t>(addr + 0x10);
				uint64_t endBlock = physmem->read<uint64_t>(addr + 0x18);
				
				Sys::stdout->write(" %3i:  0x%X - 0x%X (%i blocks)\n", i, startBlock, endBlock, endBlock - startBlock);
				
				index = physmem->read<uint32_t>(addr);
			}
		}
	}
}


bool PPCModule::compareText(Ref<PPCModule> a, Ref<PPCModule> b) {
	return a->text < b->text;
}


PPCDebugger::PPCDebugger(PhysicalMemory *physmem, PPCProcessor *cpu) {
	this->physmem = physmem;
	this->cpu = cpu;
}

Processor *PPCDebugger::getProcessor() {
	return cpu;
}

std::string PPCDebugger::name() {
	return StringUtils::format("PPC%i", cpu->core.sprs[PPCCore::PIR]);
}

Ref<EvalContext> PPCDebugger::getContext() {
	Ref<EvalContext> context = new EvalContext();
	
	for (int i = 0; i < 32; i++) {
		std::string reg = StringUtils::format("r%i", i);
		context->add(reg, cpu->core.regs[i]);
	}
	
	context->add("pc", cpu->core.pc);
	context->add("lr", cpu->core.sprs[PPCCore::LR]);
	context->add("ctr", cpu->core.sprs[PPCCore::CTR]);
	context->add("cr", cpu->core.cr);
	
	context->add("srr0", cpu->core.sprs[PPCCore::SRR0]);
	context->add("srr1", cpu->core.sprs[PPCCore::SRR1]);
	
	return context;
}

uint32_t PPCDebugger::pc() {
	return cpu->core.pc;
}

void PPCDebugger::step() {
	cpu->step();
}

bool PPCDebugger::translate(uint32_t *address) {
	return cpu->mmu.translate(address, MemoryAccess::DataRead, true);
}

void PPCDebugger::printState() {
	Sys::stdout->write(
		"r0  = %08X r1  = %08X r2  = %08X r3  = %08X r4  = %08X\n"
		"r5  = %08X r6  = %08X r7  = %08X r8  = %08X r9  = %08X\n"
		"r10 = %08X r11 = %08X r12 = %08X r13 = %08X r14 = %08X\n"
		"r15 = %08X r16 = %08X r17 = %08X r18 = %08X r19 = %08X\n"
		"r20 = %08X r21 = %08X r22 = %08X r23 = %08X r24 = %08X\n"
		"r25 = %08X r26 = %08X r27 = %08X r28 = %08X r29 = %08X\n"
		"r30 = %08X r31 = %08X pc  = %08X lr  = %08X ctr = %08X\n"
		"cr  = %08X\n",
		cpu->core.regs[0], cpu->core.regs[1], cpu->core.regs[2],
		cpu->core.regs[3], cpu->core.regs[4], cpu->core.regs[5],
		cpu->core.regs[6], cpu->core.regs[7], cpu->core.regs[8],
		cpu->core.regs[9], cpu->core.regs[10], cpu->core.regs[11],
		cpu->core.regs[12], cpu->core.regs[13], cpu->core.regs[14],
		cpu->core.regs[15], cpu->core.regs[16], cpu->core.regs[17],
		cpu->core.regs[18], cpu->core.regs[19], cpu->core.regs[20],
		cpu->core.regs[21], cpu->core.regs[22], cpu->core.regs[23],
		cpu->core.regs[24], cpu->core.regs[25], cpu->core.regs[26],
		cpu->core.regs[27], cpu->core.regs[28], cpu->core.regs[29],
		cpu->core.regs[30], cpu->core.regs[31], cpu->core.pc,
		cpu->core.sprs[PPCCore::LR], cpu->core.sprs[PPCCore::CTR],
		cpu->core.cr
	);
}

void PPCDebugger::printStateDetailed() {
	Sys::stdout->write(
		"r0  = %08X r1  = %08X r2  = %08X r3  = %08X r4  = %08X\n"
		"r5  = %08X r6  = %08X r7  = %08X r8  = %08X r9  = %08X\n"
		"r10 = %08X r11 = %08X r12 = %08X r13 = %08X r14 = %08X\n"
		"r15 = %08X r16 = %08X r17 = %08X r18 = %08X r19 = %08X\n"
		"r20 = %08X r21 = %08X r22 = %08X r23 = %08X r24 = %08X\n"
		"r25 = %08X r26 = %08X r27 = %08X r28 = %08X r29 = %08X\n"
		"r30 = %08X r31 = %08X pc  = %08X lr  = %08X ctr = %08X\n"
		"cr  = %08X xer = %08X\n",
		cpu->core.regs[0], cpu->core.regs[1], cpu->core.regs[2],
		cpu->core.regs[3], cpu->core.regs[4], cpu->core.regs[5],
		cpu->core.regs[6], cpu->core.regs[7], cpu->core.regs[8],
		cpu->core.regs[9], cpu->core.regs[10], cpu->core.regs[11],
		cpu->core.regs[12], cpu->core.regs[13], cpu->core.regs[14],
		cpu->core.regs[15], cpu->core.regs[16], cpu->core.regs[17],
		cpu->core.regs[18], cpu->core.regs[19], cpu->core.regs[20],
		cpu->core.regs[21], cpu->core.regs[22], cpu->core.regs[23],
		cpu->core.regs[24], cpu->core.regs[25], cpu->core.regs[26],
		cpu->core.regs[27], cpu->core.regs[28], cpu->core.regs[29],
		cpu->core.regs[30], cpu->core.regs[31], cpu->core.pc,
		cpu->core.sprs[PPCCore::LR], cpu->core.sprs[PPCCore::CTR],
		cpu->core.cr, cpu->core.sprs[PPCCore::XER]
	);
	Sys::stdout->write("\n");
	Sys::stdout->write(
		"srr0 = %08X srr1 = %08X dar  = %08X dsisr= %08X\n"
		"msr  = %08X dec  = %08X dabr = %08X iabr = %08X\n",
		cpu->core.sprs[PPCCore::SRR0], cpu->core.sprs[PPCCore::SRR1],
		cpu->core.sprs[PPCCore::DAR], cpu->core.sprs[PPCCore::DSISR],
		cpu->core.msr, cpu->core.sprs[PPCCore::DEC],
		cpu->core.sprs[PPCCore::DABR], cpu->core.sprs[PPCCore::IABR]
	);
	Sys::stdout->write("\n");
	Sys::stdout->write(
		"hid0 = %08X hid1 = %08X hid2 = %08X hid4 = %08X hid5 = %08X\n"
		"pcsr = %08X scr  = %08X car  = %08X bcr  = %08X pir  = %08X\n"
		"pvr  = %08X wpar = %08X wpsar= %08X\n",
		cpu->core.sprs[PPCCore::HID0], cpu->core.sprs[PPCCore::HID1],
		cpu->core.sprs[PPCCore::HID2], cpu->core.sprs[PPCCore::HID4],
		cpu->core.sprs[PPCCore::HID5], cpu->core.sprs[PPCCore::PCSR],
		cpu->core.sprs[PPCCore::SCR], cpu->core.sprs[PPCCore::CAR],
		cpu->core.sprs[PPCCore::BCR], cpu->core.sprs[PPCCore::PIR],
		cpu->core.sprs[PPCCore::PVR], cpu->core.sprs[PPCCore::WPAR],
		cpu->core.sprs[PPCCore::WPSAR]
	);
}

void PPCDebugger::printStackTrace() {
	ModuleList modules = getModules();
	
	uint32_t sp = cpu->core.regs[1];
	uint32_t lr = cpu->core.sprs[PPCCore::LR];
	uint32_t pc = cpu->core.pc;

	Sys::stdout->write("\n");
	Sys::stdout->write("pc = %s\n", formatAddress(pc, modules));
	Sys::stdout->write("lr = %s\n", formatAddress(lr, modules));
	Sys::stdout->write("\n");
	
	translate(&sp);
	sp = physmem->read<uint32_t>(sp);
	
	printStackTraceForThread(sp, modules);
}

void PPCDebugger::printStackTraceForThread(uint32_t sp, ModuleList &modules) {
	Sys::stdout->write("Stack trace:\n");
	while (sp) {
		translate(&sp);
		
		uint32_t lr = physmem->read<uint32_t>(sp + 4);
		if (!lr) break;
		
		Sys::stdout->write("    %s\n", formatAddress(lr, modules));

		sp = physmem->read<uint32_t>(sp);
	}
}

MemoryRegion::Access AccessProt[] = {
	MemoryRegion::ReadWrite, MemoryRegion::ReadWrite,
	MemoryRegion::ReadWrite, MemoryRegion::ReadOnly,
	MemoryRegion::NoAccess, MemoryRegion::ReadOnly,
	MemoryRegion::ReadWrite, MemoryRegion::ReadOnly
};

void PPCDebugger::printMemoryMap() {
	Sys::stdout->write("DBAT:\n");
	for (int i = 0; i < 8; i++) {
		uint32_t batu = cpu->core.sprs[PPCCore::DBAT0U + i * 2 + i / 4 * 8];
		uint32_t batl = cpu->core.sprs[PPCCore::DBAT0L + i * 2 + i / 4 * 8];
		Sys::stdout->write("    dbat%i: ", i);
		printBAT(batu, batl);
	}
	
	Sys::stdout->write("\nIBAT:\n");
	for (int i = 0; i < 8; i++) {
		uint32_t batu = cpu->core.sprs[PPCCore::IBAT0U + i * 2 + i / 4 * 8];
		uint32_t batl = cpu->core.sprs[PPCCore::IBAT0L + i * 2 + i / 4 * 8];
		Sys::stdout->write("    ibat%i: ", i);
		printBAT(batu, batl);
	}
	
	uint32_t sdr1 = cpu->core.sprs[PPCCore::SDR1];
	uint32_t pagetbl = sdr1 & 0xFFFF0000;
	uint32_t pagemask = sdr1 & 0x1FF;
	uint32_t hashmask = (pagemask << 10) | 0x3FF;
	
	Sys::stdout->write("\nPage table (%08X):\n", pagetbl);
	
	if (pagetbl >= 0xFFE00000) {
		pagetbl = pagetbl - 0xFFE00000 + 0x08000000;
	}
	
	MemoryMap memory(true);
	
	for (uint32_t segment = 0; segment < 16; segment++) {
		uint32_t sr = cpu->core.sr[segment];
		if (sr >> 31) continue;
		
		bool ks = (sr >> 30) & 1;
		bool kp = (sr >> 29) & 1;
		bool nx = (sr >> 28) & 1;
		uint32_t vsid = sr & 0xFFFFFF;
		
		for (uint32_t pageidx = 0; pageidx < 2048; pageidx++) {
			uint32_t hash = ((vsid & 0x7FFFF) ^ pageidx) & hashmask;
			uint32_t api = pageidx >> 5;
			
			uint32_t pteaddr = pagetbl | (hash << 6);
			for (int i = 0; i < 8; i++) {
				uint64_t pte = physmem->read<uint64_t>(pteaddr + i * 8);
				if (!(pte >> 63)) continue;
				if ((pte >> 38) & 1) continue;
				if (((pte >> 39) & 0xFFFFFF) != vsid) continue;
				if (((pte >> 32) & 0x3F) != api) continue;
				
				int pp = pte & 3;
				
				Ref<MemoryRegion> region = new MemoryRegion();
				region->virt = (segment << 28) | (pageidx << 17);
				region->phys = pte & 0xFFFFF000;
				region->size = 0x20000;
				region->user = AccessProt[kp * 4 + pp];
				region->system = AccessProt[ks * 4 + pp];
				region->executable = !nx;
				memory.add(region);
			}
		}
	}
	
	memory.print();
}

const char *BATValidity[] = {
	"user mode only", "supervisor only", "user/supervisor"
};

const char *BATAccess[] = {
	"no access", "read only", "read/write", "read only"
};

void PPCDebugger::printBAT(uint32_t batu, uint32_t batl) {
	if (batu & 3) {
		char caching[5] = "WIMG";
		for (int i = 0; i < 4; i++) {
			if (!(batl & (1 << (6 - i)))) {
				caching[i] = '-';
			}
		}
		
		const char *access = BATAccess[batl & 3];
		const char *validity = BATValidity[(batu & 3) - 1];
		
		uint32_t bl = (batu >> 2) & 0x7FF;
		uint32_t size = (bl + 1) << 17;
		uint64_t vaddr = batu & 0xFFFE0000;
		uint64_t paddr = batl & 0xFFFE0000;
		
		Sys::stdout->write(
			"%08X-%08X => %08X-%08X (%s, %s, %s)\n",
			vaddr, vaddr + size, paddr, paddr + size,
			caching, access, validity
		);
	}
	else {
		Sys::stdout->write("disabled\n");
	}
}

void PPCDebugger::printModules() {
	ModuleList modules = getModules();
	std::sort(modules.begin(), modules.end(), PPCModule::compareText);
	
	for (Ref<PPCModule> module : modules) {
		Sys::stdout->write(" %08X: %s\n", module->text, module->path);
	}
}

void PPCDebugger::printModuleDetails(std::string name) {
	ModuleList modules = getModules();
	for (Ref<PPCModule> module : modules) {
		if (module->path.find(name) != std::string::npos) {
			Sys::stdout->write("%s:\n", module->path);
			Sys::stdout->write("    .text: %08X - %08X\n", module->text, module->text + module->textsize);
			Sys::stdout->write("    .data: %08X - %08X\n", module->data, module->data + module->datasize);
		}
	}
}

const char *ThreadAffinity[] = {
	"invalid", "core 0", "core 1", "core 0/1",
	"core 2", "core 0/2", "core 1/2", "any core"
};

void PPCDebugger::printThreads() {
	if (!(cpu->core.msr & 0x10)) return;
	
	ModuleList modules = getModules();
	
	uint32_t thread = 0x100567F8;
	if (!translate(&thread)) return;
	
	thread = physmem->read<uint32_t>(thread);
	while (thread) {
		uint32_t addr = thread;
		if (!translate(&thread)) break;
		
		uint32_t entryPoint = physmem->read<uint32_t>(thread + 0x39C);
		
		std::string owner;
		for (Ref<PPCModule> module : modules) {
			if (module->text <= entryPoint && entryPoint < module->text + module->textsize) {
				owner = module->name;
			}
		}
		
		std::string name = "<no name>";
		
		uint32_t nameptr = physmem->read<uint32_t>(thread + 0x5C0);
		if (nameptr) {
			if (!translate(&nameptr)) break;
			
			name = physmem->read<std::string>(nameptr);
		}
		
		uint32_t affinity = physmem->read<uint32_t>(thread + 0x304) & 7;
		std::string core = StringUtils::format("[%s]", ThreadAffinity[affinity]);
		
		uint8_t stateId = physmem->read<uint8_t>(thread + 0x324);
		
		std::string stateName = "invalid";
		if (stateId == 1) stateName = "ready";
		else if (stateId == 2) stateName = "running";
		else if (stateId == 4) stateName = "waiting";
		else if (stateId == 8) stateName = "dead";
		
		std::string state = StringUtils::format("(%s)", stateName);
		
		Sys::stdout->write(" %08X:  %-12s  %-10s %-9s  %s\n", addr, owner, core, state, name);
		
		thread = physmem->read<uint32_t>(thread + 0x38C);
	}
}

void PPCDebugger::printThreadDetails(uint32_t thread) {
	if (!(cpu->core.msr & 0x10)) {
		Sys::stdout->write("MMU is disabled.\n");
		return;
	}
	
	if (!translate(&thread)) {
		Sys::stdout->write("Address translation failed.\n");
		return;
	}
	
	uint32_t tag = physmem->read<uint32_t>(thread + 0x320);
	if (tag != 0x74487244) { //tHrD
		Sys::stdout->write("This is not a valid thread.\n");
		return;
	}
	
	ModuleList modules = getModules();
	
	std::string name;
	uint32_t nameptr = physmem->read<uint32_t>(thread + 0x5C0);
	if (nameptr) {
		translate(&nameptr);
		name = physmem->read<std::string>(nameptr);
	}
	
	uint32_t sp = physmem->read<uint32_t>(thread + 0xC);
	uint32_t stackBase = physmem->read<uint32_t>(thread + 0x394);
	uint32_t stackTop = physmem->read<uint32_t>(thread + 0x398);
	uint32_t entryPoint = physmem->read<uint32_t>(thread + 0x39C);
	
	Sys::stdout->write("Name: %s\n\n", name);
	Sys::stdout->write("Entry point: %s\n\n", formatAddress(entryPoint, modules));
	Sys::stdout->write("Stack: 0x%08X - 0x%08X (0x%X bytes)\n\n", stackTop, stackBase, stackBase - stackTop);
	printStackTraceForThread(sp, modules);
	Sys::stdout->write("\n");
	
	uint8_t state = physmem->read<uint8_t>(thread + 0x324);
	if (state == 1) Sys::stdout->write("This thread is ready to run.\n");
	else if (state == 2) Sys::stdout->write("This thread is running right now.\n");
	else if (state == 4) printThreadWait(thread);
	else if (state == 8) Sys::stdout->write("This thread is dead.\n");
	else {
		Sys::stdout->write("This thread has an invalid state.\n");
	}
}

void PPCDebugger::printThreadWait(uint32_t thread) {
	uint32_t queue = physmem->read<uint32_t>(thread + 0x35C);
	if (queue && translate(&queue)) {
		uint32_t parent = physmem->read<uint32_t>(queue + 8);
		if (parent && translate(&parent)) {
			std::string name = "<no name>";
			uint32_t namePtr = physmem->read<uint32_t>(parent + 4);
			if (namePtr && translate(&namePtr)) {
				name = physmem->read<std::string>(namePtr);
			}
			
			uint32_t tag = physmem->read<uint32_t>(parent);
			if (tag == 0x65566E54) Sys::stdout->write("This thread is waiting for an event: %s\n", name);
			else if (tag == 0x614C724D) Sys::stdout->write("This thread is waiting for an alarm: %s\n", name);
			else if (tag == 0x6D557458) Sys::stdout->write("This thread is waiting for a mutex: %s\n", name);
			else if (tag == 0x634E6456) Sys::stdout->write("This thread is waiting for a condition variable: %s\n", name);
			else if (tag == 0x73506852) Sys::stdout->write("This thread is waiting for a semaphore: %s\n", name);
			else if (tag == 0x6D536751) Sys::stdout->write("This thread is waiting for a message queue: %s\n", name);
			else if (tag == 0x614C6D51) Sys::stdout->write("This thread is waiting for an alarm queue: %s\n", name);
			else {
				Sys::stdout->write("This thread is currently waiting.\n");
			}
			return;
		}
	}
	Sys::stdout->write("This thread is currently waiting.\n");
}

std::string PPCDebugger::formatAddress(uint32_t addr, ModuleList &modules) {
	for (Ref<PPCModule> module : modules) {
		if (module->text <= addr && addr <= module->text + module->textsize) {
			return StringUtils::format("%08X: %s:text+0x%X", addr, module->name, addr - module->text);
		}
		if (module->data <= addr && addr <= module->data + module->datasize) {
			return StringUtils::format("%08X: %s:data+0x%X", addr, module->name, addr - module->data);
		}
	}
	return StringUtils::format("%08X", addr);
}

PPCDebugger::ModuleList PPCDebugger::getModules() {
	ModuleList modules;
	
	uint32_t addr = 0x10081018;
	if ((cpu->core.msr & 0x10) && translate(&addr)) {
		addr = physmem->read<uint32_t>(addr);
		while (addr) {
			if (!translate(&addr)) break;
			
			uint32_t info = physmem->read<uint32_t>(addr + 0x28);
			if (!translate(&info)) break;
			
			uint32_t pathAddr = physmem->read<uint32_t>(info);
			if (!translate(&pathAddr)) break;
			
			Ref<PPCModule> module = new PPCModule();
			module->path = physmem->read<std::string>(pathAddr);
			module->name = module->path.substr(module->path.rfind('\\') + 1);
			module->text = physmem->read<uint32_t>(info + 4);
			module->textsize = physmem->read<uint32_t>(info + 0xC);
			module->data = physmem->read<uint32_t>(info + 0x10);
			module->datasize = physmem->read<uint32_t>(info + 0x18);
			modules.push_back(module);
			
			addr = physmem->read<uint32_t>(addr + 0x54);
		}
	}
	
	return modules;
}


Debugger::Debugger(Emulator *emulator) {
	this->emulator = emulator;
	this->physmem = &emulator->physmem;
	
	armcpu = &emulator->armcpu;
	ppccpu[0] = &emulator->ppc[0];
	ppccpu[1] = &emulator->ppc[1];
	ppccpu[2] = &emulator->ppc[2];
	
	armdebug = new ARMDebugger(&emulator->physmem, armcpu);
	ppcdebug[0] = new PPCDebugger(&emulator->physmem, ppccpu[0]);
	ppcdebug[1] = new PPCDebugger(&emulator->physmem, ppccpu[1]);
	ppcdebug[2] = new PPCDebugger(&emulator->physmem, ppccpu[2]);
	
	core = 0;
}

Ref<DebugInterface> Debugger::getCurrent() {
	if (core == 0) return armdebug;
	if (core == 1) return ppcdebug[0];
	if (core == 2) return ppcdebug[1];
	if (core == 3) return ppcdebug[2];
	return nullptr;
}

Processor *Debugger::getProcessor() {
	return getCurrent()->getProcessor();
}

void Debugger::show(int core) {
	emulator->pause();
	
	if (core != -1) {
		this->core = core;
	}
	
	debugging = true;
	while (debugging) {
		DebugInterface *debugger = getCurrent();
		
		Sys::stdout->write("%s:%08X> ", debugger->name(), debugger->pc());
		
		std::string line = Sys::stdin->readline();
		
		std::vector<std::string> args;
		
		std::string current;
		for (char c : line) {
			if (c == ' ') {
				if (!current.empty()) {
					args.push_back(current);
					current.clear();
				}
			}
			else {
				current += c;
			}
		}
		if (!current.empty()) {
			args.push_back(current);
		}
		
		if (args.size() > 0) {
			ArgParser parser(debugger->getContext(), args);
			processCommand(args[0], &parser);
		}
	}
}

void Debugger::processCommand(std::string command, ArgParser *args) {
	if (command == "help") help(args);
	else if (command == "exit") quit(args);
	else if (command == "quit") quit(args);
	else if (command == "select") select(args);
	
	else if (command == "step") step(args);
	else if (command == "run") run(args);
	else if (command == "reset") reset(args);
	else if (command == "restart") restart(args);
	#if STATS
	else if (command == "stats") stats(args);
	#endif
	#if METRICS
	else if (command == "metrics") metrics(args);
	else if (command == "syscalls") syscalls(args);
	#endif
	
	#if BREAKPOINTS
	else if (command == "break") breakp(args);
	#endif
	#if WATCHPOINTS
	else if (command == "watch") watch(args);
	#endif
	
	else if (command == "state") state(args);
	else if (command == "print") print(args);
	else if (command == "trace") trace(args);
	
	else if (command == "read") read(args);
	else if (command == "translate") translate(args);
	else if (command == "memmap") memmap(args);
	
	else if (command == "modules") modules(args);
	else if (command == "module") module(args);
	else if (command == "threads") threads(args);
	else if (command == "thread") thread(args);
	else if (command == "queues") queues(args);
	else if (command == "queue") queue(args);
	else if (command == "devices") devices(args);
	else if (command == "hardware") hardware(args);
	
	else if (command == "volumes") volumes(args);
	else if (command == "fileclients") fileclients(args);
	else if (command == "slccache") slccache(args);
	
	else {
		Sys::stdout->write("Unknown command.\n");
	}
}

void Debugger::help(ArgParser *args) {
	if (!args->finish()) return;
	Sys::stdout->write(HELP_TEXT);
}

void Debugger::quit(ArgParser *args) {
	if (!args->finish()) return;
	emulator->quit();
	debugging = false;
}

void Debugger::select(ArgParser *args) {
	std::string name;
	if (!args->string(&name)) return;
	if (!args->finish()) return;
	
	if (name == "arm") core = 0;
	else if (name == "ppc0") core = 1;
	else if (name == "ppc1") core = 2;
	else if (name == "ppc2") core = 3;
	else {
		Sys::stdout->write("Please provide a valid processor name.\n");
	}
}

void Debugger::step(ArgParser *args) {
	uint32_t count = 1;
	if (!args->eof()) {
		if (!args->integer(&count)) return;
		if (!args->finish()) return;
	}
	
	for (uint32_t i = 0; i < count; i++) {
		getCurrent()->step();
	}
}

void Debugger::run(ArgParser *args) {
	if (!args->finish()) return;
	debugging = false;
}

void Debugger::reset(ArgParser *args) {
	if (!args->finish()) return;
	emulator->reset();
}

void Debugger::restart(ArgParser *args) {
	if (!args->finish()) return;
	emulator->reset();
	debugging = false;
}

#if STATS
void Debugger::stats(ArgParser *args) {
	if (!args->finish()) return;
	Sys::stdout->write("ARM:\n");
	Sys::stdout->write("    ARM instrs executed:       %i\n", armcpu->armInstrs);
	Sys::stdout->write(
		"    ARM instrs executed (jit): %i (%i%%)\n",
		armcpu->jit.instrsExecuted, percentage(armcpu->jit.instrsExecuted, armcpu->armInstrs)
	);
	Sys::stdout->write(
		"    ARM instrs compiled (jit): %i (avg usage: %i)\n",
		armcpu->jit.instrsCompiled, divide(armcpu->jit.instrsExecuted, armcpu->jit.instrsCompiled)
	);
	Sys::stdout->write(
		"    ARM jit memory usage:      %i bytes (%i per instr)\n",
		armcpu->jit.instrSize, divide(armcpu->jit.instrSize, armcpu->jit.instrsCompiled)
	);
	Sys::stdout->write("    \n");
	Sys::stdout->write("    Thumb instrs executed:       %i\n", armcpu->thumbInstrs);
	Sys::stdout->write(
		"    Thumb instrs executed (jit): %i (%i%%)\n",
		armcpu->thumbJit.instrsExecuted, percentage(armcpu->thumbJit.instrsExecuted, armcpu->thumbInstrs)
	);
	Sys::stdout->write(
		"    Thumb instrs compiled (jit): %i (avg usage: %i)\n",
		armcpu->thumbJit.instrsCompiled, divide(armcpu->thumbJit.instrsExecuted, armcpu->thumbJit.instrsCompiled)
	);
	Sys::stdout->write(
		"    Thumb jit memory usage:      %i bytes (%i per instr)\n",
		armcpu->thumbJit.instrSize, divide(armcpu->thumbJit.instrSize, armcpu->thumbJit.instrsCompiled)
	);
	Sys::stdout->write("    \n");
	Sys::stdout->write("    Number of data reads:  %i\n", armcpu->dataReads);
	Sys::stdout->write("    Number of data writes: %i\n", armcpu->dataWrites);
	Sys::stdout->write("    \n");
	Sys::stdout->write("    MMU cache hits:   %i\n", armcpu->mmu.cache.hits);
	Sys::stdout->write("    MMU cache misses: %i\n", armcpu->mmu.cache.misses);
	printPPCStats(0);
	printPPCStats(1);
	printPPCStats(2);
}

void Debugger::printPPCStats(int index) {
	Sys::stdout->write("\n");
	Sys::stdout->write("PPC %i:\n", index);
	Sys::stdout->write("    Instructions executed:       %i\n", ppccpu[index]->instrsExecuted);
	Sys::stdout->write(
		"    Instructions executed (jit): %i (%i%%)\n", ppccpu[index]->jit.instrsExecuted,
		percentage(ppccpu[index]->jit.instrsExecuted, ppccpu[index]->instrsExecuted)
	);
	Sys::stdout->write(
		"    Instructions compiled (jit): %i (avg usage: %i)\n", ppccpu[index]->jit.instrsCompiled,
		divide(ppccpu[index]->jit.instrsExecuted, ppccpu[index]->jit.instrsCompiled)
	);
	Sys::stdout->write(
		"    JIT memory usage:            %i bytes (%i per instr)\n", ppccpu[index]->jit.instrSize,
		divide(ppccpu[index]->jit.instrSize, ppccpu[index]->jit.instrsCompiled)
	);
	Sys::stdout->write("    \n");
	Sys::stdout->write("    MMU cache hits:   %i\n", ppccpu[index]->mmu.cache.hits);
	Sys::stdout->write("    MMU cache misses: %i\n", ppccpu[index]->mmu.cache.misses);
	Sys::stdout->write("    \n");
	Sys::stdout->write("    DSI exceptions    %i\n", ppccpu[index]->core.dsiExceptions);
	Sys::stdout->write("    ISI exceptions:   %i\n", ppccpu[index]->core.isiExceptions);
	Sys::stdout->write("    External intr:    %i\n", ppccpu[index]->core.externalInterrupts);
	Sys::stdout->write("    Decrementer intr: %i\n", ppccpu[index]->core.decrementerInterrupts);
	Sys::stdout->write("    System calls:     %i\n", ppccpu[index]->core.systemCalls);
}
#endif

#if METRICS
void Debugger::metrics(ArgParser *args) {
	std::string name;
	std::string mode;
	if (!args->string(&name)) return;
	if (!args->string(&mode)) return;
	if (!args->finish()) return;
	
	PPCMetrics::PrintMode pmode;
	if (mode == "category") pmode = PPCMetrics::CATEGORY;
	else if (mode == "frequency") pmode = PPCMetrics::FREQUENCY;
	else {
		Sys::stdout->write("Please provide a valid sort order.\n");
		return;
	}
	
	if (name == "ppc0") ppccpu[0]->metrics.print(pmode);
	else if (name == "ppc1") ppccpu[1]->metrics.print(pmode);
	else if (name == "ppc2") ppccpu[2]->metrics.print(pmode);
	else {
		Sys::stdout->write("Please provide a valid processor name.\n");
	}
}

void Debugger::syscalls(ArgParser *args) {
	std::string name;
	if (!args->string(&name)) return;
	if (!args->finish()) return;
	
	if (name == "ppc0") printSyscalls(0);
	else if (name == "ppc1") printSyscalls(1);
	else if (name == "ppc2") printSyscalls(2);
	else {
		Sys::stdout->write("Please provide a valid processor name.\n");
	}
}

bool compareFreq(std::pair<uint32_t, uint64_t> a, std::pair<uint32_t, uint64_t> b) {
	return a.second > b.second || (a.second == b.second && a.first < b.first);
}

void Debugger::printSyscalls(int core) {
	const std::map<uint32_t, uint64_t> &syscalls = ppccpu[core]->core.syscallIds;
	std::vector<std::pair<uint32_t, uint64_t>> list(syscalls.begin(), syscalls.end());
	std::sort(list.begin(), list.end(), compareFreq);
	
	uint64_t total = 0;
	for (std::pair<uint32_t, uint64_t> pair : list) {
		total += pair.second;
	}
	
	Sys::stdout->write("Syscalls executed: %i\n", total);
	Sys::stdout->write("\n");
	
	Sys::stdout->write("Sorted by frequency:\n");
	for (std::pair<uint32_t, uint64_t> instr : list) {
		Sys::stdout->write(
			"    0x%04X: %6i (%2i%%)\n", instr.first,
			instr.second, percentage(instr.second, total)
		);
	}
}
#endif

#if BREAKPOINTS
void Debugger::breakp(ArgParser *args) {
	std::string command;
	if (!args->string(&command)) return;
	
	Processor *cpu = getProcessor();
	if (command == "list") {
		if (!args->finish()) return;
		
		std::vector<uint32_t> breakpoints = cpu->breakpoints;
		std::sort(breakpoints.begin(), breakpoints.end());
		
		if (breakpoints.size() == 0) {
			Sys::stdout->write("No breakpoints are installed.\n");
		}
		else {
			if (breakpoints.size() == 1) {
				Sys::stdout->write("1 breakpoint:\n");
			}
			else {
				Sys::stdout->write("%i breakpoints:\n", breakpoints.size());
			}
			for (uint32_t bp : breakpoints) {
				Sys::stdout->write("    0x%08X\n", bp);
			}
		}
	}
	else if (command == "clear") {
		if (!args->finish()) return;
		
		if (core == 0) armcpu->breakpoints.clear();
		else {
			for (int i = 0; i < 3; i++) {
				ppccpu[i]->breakpoints.clear();
			}
		}
	}
	else if (command == "add" || command == "del") {
		uint32_t address;
		if (!args->integer(&address)) return;
		if (!args->finish()) return;
		
		bool exists = cpu->isBreakpoint(address);
		if (command == "add") {
			if (exists) {
				Sys::stdout->write("Breakpoint at 0x%X already exists.\n", address);
			}
			else {
				Sys::stdout->write("Added breakpoint at 0x%X.\n", address);
				if (core == 0) armcpu->addBreakpoint(address);
				else {
					for (int i = 0; i < 3; i++) {
						ppccpu[i]->addBreakpoint(address);
					}
				}
			}
		}
		else {
			if (!exists) {
				Sys::stdout->write("Breakpoint at 0x%X does not exists.\n", address);
			}
			else {
				Sys::stdout->write("Removed breakpoint at 0x%X.\n", address);
				if (core == 0) armcpu->removeBreakpoint(address);
				else {
					for (int i = 0; i < 3; i++) {
						ppccpu[i]->removeBreakpoint(address);
					}
				}
			}
		}
	}
	else {
		Sys::stdout->write("Unknown breakpoint command: %s\n", command);
	}
}
#endif

#if WATCHPOINTS
void Debugger::watch(ArgParser *args) {
	std::string command;
	if (!args->string(&command)) return;
	
	Processor *cpu = getProcessor();
	if (command == "list") {
		if (!args->finish()) return;
		
		size_t total = 0;
		for (int i = 0; i < 4; i++) {
			total += cpu->watchpoints[i / 2][i % 2].size();
		}
		
		if (total == 0) {
			Sys::stdout->write("No watchpoints are installed.\n");
		}
		else {
			if (total == 1) {
				Sys::stdout->write("1 watchpoint:\n");
			}
			else {
				Sys::stdout->write("%i watchpoints:\n", total);
			}
			
			for (int virt = 0; virt < 2; virt++) {
				for (int write = 0; write < 2; write++) {
					for (uint32_t wp : cpu->watchpoints[write][virt]) {
						Sys::stdout->write(
							"    0x%08X (%s, %s)\n", wp,
							virt ? "virtual" : "physical",
							write ? "write" : "read"
						);
					}
				}
			}
		}
	}
	else if (command == "clear") {
		if (!args->finish()) return;
		
		for (int i = 0; i < 4; i++) {
			cpu->watchpoints[i / 2][i % 2].clear();
		}
	}
	else if (command == "add" || command == "del") {
		std::string mode, type;
		uint32_t address;
		
		if (!args->string(&mode)) return;
		if (!args->string(&type)) return;
		if (!args->integer(&address)) return;
		if (!args->finish()) return;
		
		if (mode != "phys" && mode != "virt") {
			Sys::stdout->write("Please specify either 'phys' or 'virt'.\n");
			return;
		}
		
		if (type != "read" && type != "write") {
			Sys::stdout->write("Please specify either 'read' or 'write'.\n");
			return;
		}
		
		bool write = type == "write";
		bool virt = mode == "virt";
		
		mode = virt ? "virtual" : "physical";
		
		bool exists = cpu->isWatchpoint(write, virt, address, 1);
		if (command == "add") {
			if (exists) {
				Sys::stdout->write("Watchpoint (%s) at %s address 0x%X already exists.\n", type, mode, address);
			}
			else {
				Sys::stdout->write("Added watchpoint (%s) at %s address 0x%X.\n", type, mode, address);
				cpu->addWatchpoint(write, virt, address);
			}
		}
		else {
			if (!exists) {
				Sys::stdout->write("Watchpoint (%s) at %s address 0x%X does not exists.\n", type, mode, address);
			}
			else {
				Sys::stdout->write("Removed watchpoint (%s) at %s address 0x%X.\n", type, mode, address);
				cpu->removeWatchpoint(write, virt, address);
			}
		}
	}
	else {
		Sys::stdout->write("Unknown watchpoint command: %s\n", command);
	}
}
#endif

void Debugger::state(ArgParser *args) {
	if (!args->eof()) {
		std::string param;
		if (!args->string(&param)) return;
		if (!args->finish()) return;
		
		if (param != "full") {
			Sys::stdout->write("Invalid argument.");
			return;
		}
		
		getCurrent()->printStateDetailed();
	}
	else {
		getCurrent()->printState();
	}
}

void Debugger::print(ArgParser *args) {
	uint32_t value;
	if (!args->integer(&value)) return;
	if (!args->finish()) return;
	
	Sys::stdout->write("0x%X (%i)\n", value, value);
}

void Debugger::trace(ArgParser *args) {
	if (!args->finish()) return;
	getCurrent()->printStackTrace();
}

void Debugger::read(ArgParser *args) {
	std::string mode;
	uint32_t address, length;
	if (!args->string(&mode)) return;
	if (!args->integer(&address)) return;
	if (!args->integer(&length)) return;
	if (!args->finish()) return;
	
	if (mode == "virt") {
		if (!getCurrent()->translate(&address)) {
			Sys::stdout->write("Address translation failed.\n");
			return;
		}
	}
	else if (mode != "phys") {
		Sys::stdout->write("Please specify either 'phys' or 'virt'.\n");
		return;
	}
	
	Buffer buffer = physmem->read(address, length);
	
	std::string text = buffer.tostring();
	for (int i = 0; i < text.size(); i++) {
		if (!StringUtils::is_printable(text[i])) {
			text[i] = ' ';
		}
	}
	
	Sys::stdout->write("%s\n\n%s\n", buffer.hexstring(), text);
}

void Debugger::translate(ArgParser *args) {
	uint32_t addr;
	if (!args->integer(&addr)) return;
	if (!args->finish()) return;
	
	if (getCurrent()->translate(&addr)) {
		Sys::stdout->write("0x%08X\n", addr);
	}
	else {
		Sys::stdout->write("Address translation failed.\n");
	}
}

void Debugger::memmap(ArgParser *args) {
	if (!args->finish()) return;
	getCurrent()->printMemoryMap();
}

void Debugger::modules(ArgParser *args) {
	if (!args->finish()) return;
	ppcdebug[1]->printModules();
}

void Debugger::module(ArgParser *args) {
	std::string name;
	if (!args->string(&name)) return;
	if (!args->finish()) return;
	ppcdebug[1]->printModuleDetails(name);
}

void Debugger::threads(ArgParser *args) {
	if (!args->finish()) return;
	getCurrent()->printThreads();
}

void Debugger::thread(ArgParser *args) {
	uint32_t tid;
	if (!args->integer(&tid)) return;
	if (!args->finish()) return;
	getCurrent()->printThreadDetails(tid);
}

void Debugger::queues(ArgParser *args) {
	if (!args->finish()) return;
	armdebug->printMessageQueues();
}

void Debugger::queue(ArgParser *args) {
	uint32_t id;
	if (!args->integer(&id)) return;
	if (!args->finish()) return;
	armdebug->printMessageQueueDetails(id);
}

void Debugger::devices(ArgParser *args) {
	if (!args->finish()) return;
	armdebug->printDevices();
}

void Debugger::hardware(ArgParser *args) {
	if (!args->finish()) return;
	Sys::stdout->write("PI_CPU0_INTMR = 0x%08X\n", physmem->read<uint32_t>(0xC000078));
	Sys::stdout->write("PI_CPU0_INTSR = 0x%08X\n", physmem->read<uint32_t>(0xC00007C)); 
	Sys::stdout->write("PI_CPU1_INTMR = 0x%08X\n", physmem->read<uint32_t>(0xC000080));
	Sys::stdout->write("PI_CPU1_INTSR = 0x%08X\n", physmem->read<uint32_t>(0xC000084));
	Sys::stdout->write("PI_CPU2_INTMR = 0x%08X\n", physmem->read<uint32_t>(0xC000088));
	Sys::stdout->write("PI_CPU2_INTSR = 0x%08X\n", physmem->read<uint32_t>(0xC00008C));
	Sys::stdout->write("\n");
	Sys::stdout->write("WG_CPU0_BASE = 0x%08X\n", physmem->read<uint32_t>(0xC000040));
	Sys::stdout->write("WG_CPU0_TOP = 0x%08X\n", physmem->read<uint32_t>(0xC000044));
	Sys::stdout->write("WG_CPU0_PTR = 0x%08X\n", physmem->read<uint32_t>(0xC000048));
	Sys::stdout->write("WG_CPU0_THRESHOLD = 0x%08X\n", physmem->read<uint32_t>(0xC00004C));
	Sys::stdout->write("WG_CPU1_BASE = 0x%08X\n", physmem->read<uint32_t>(0xC000050));
	Sys::stdout->write("WG_CPU1_TOP = 0x%08X\n", physmem->read<uint32_t>(0xC000054));
	Sys::stdout->write("WG_CPU1_PTR = 0x%08X\n", physmem->read<uint32_t>(0xC000058));
	Sys::stdout->write("WG_CPU1_THRESHOLD = 0x%08X\n", physmem->read<uint32_t>(0xC00005C));
	Sys::stdout->write("WG_CPU2_BASE = 0x%08X\n", physmem->read<uint32_t>(0xC000060));
	Sys::stdout->write("WG_CPU2_TOP = 0x%08X\n", physmem->read<uint32_t>(0xC000064));
	Sys::stdout->write("WG_CPU2_PTR = 0x%08X\n", physmem->read<uint32_t>(0xC000068));
	Sys::stdout->write("WG_CPU2_THRESHOLD = 0x%08X\n", physmem->read<uint32_t>(0xC00006C));
	Sys::stdout->write("\n");
	Sys::stdout->write("LT_ARM_INTMR_ALL = 0x%08X\n", physmem->read<uint32_t>(0xD000478));
	Sys::stdout->write("LT_ARM_INTSR_ALL = 0x%08X\n", physmem->read<uint32_t>(0xD000470));
	Sys::stdout->write("LT_ARM_INTMR_LT = 0x%08X\n", physmem->read<uint32_t>(0xD00047C));
	Sys::stdout->write("LT_ARM_INTSR_LT = 0x%08X\n", physmem->read<uint32_t>(0xD000474));
	Sys::stdout->write("LT_PPC0_INTMR_ALL = 0x%08X\n", physmem->read<uint32_t>(0xD000448));
	Sys::stdout->write("LT_PPC0_INTSR_ALL = 0x%08X\n", physmem->read<uint32_t>(0xD000440));
	Sys::stdout->write("LT_PPC0_INTMR_LT = 0x%08X\n", physmem->read<uint32_t>(0xD00044C));
	Sys::stdout->write("LT_PPC0_INTSR_LT = 0x%08X\n", physmem->read<uint32_t>(0xD000444));
	Sys::stdout->write("LT_PPC1_INTMR_ALL = 0x%08X\n", physmem->read<uint32_t>(0xD000458));
	Sys::stdout->write("LT_PPC1_INTSR_ALL = 0x%08X\n", physmem->read<uint32_t>(0xD000450));
	Sys::stdout->write("LT_PPC1_INTMR_LT = 0x%08X\n", physmem->read<uint32_t>(0xD00045C));
	Sys::stdout->write("LT_PPC1_INTSR_LT = 0x%08X\n", physmem->read<uint32_t>(0xD000454));
	Sys::stdout->write("LT_PPC2_INTMR_ALL = 0x%08X\n", physmem->read<uint32_t>(0xD000468));
	Sys::stdout->write("LT_PPC2_INTSR_ALL = 0x%08X\n", physmem->read<uint32_t>(0xD000460));
	Sys::stdout->write("LT_PPC2_INTMR_LT = 0x%08X\n", physmem->read<uint32_t>(0xD00046C));
	Sys::stdout->write("LT_PPC2_INTSR_LT = 0x%08X\n", physmem->read<uint32_t>(0xD000464));
	Sys::stdout->write("\n");
	Sys::stdout->write("IH_RB_BASE = 0x%08X\n", physmem->read<uint32_t>(0xC203E04));
	Sys::stdout->write("IH_RB_RPTR = 0x%08X\n", physmem->read<uint32_t>(0xC203E08));
	Sys::stdout->write("IH_RB_WPTR_ADDR_LO = 0x%08X\n", physmem->read<uint32_t>(0xC203E14));
	Sys::stdout->write("CP_RB_BASE = 0x%08X\n", physmem->read<uint32_t>(0xC20C100));
	Sys::stdout->write("CP_RB_RPTR_ADDR = 0x%08X\n", physmem->read<uint32_t>(0xC20C10C));
	Sys::stdout->write("CP_RB_WPTR = 0x%08X\n", physmem->read<uint32_t>(0xC20C114));
	Sys::stdout->write("DRMDMA_RB_CNTL = 0x%08X\n", physmem->read<uint32_t>(0xC20D000));
	Sys::stdout->write("DRMDMA_RB_BASE = 0x%08X\n", physmem->read<uint32_t>(0xC20D004));
	Sys::stdout->write("DRMDMA_RB_RPTR = 0x%08X\n", physmem->read<uint32_t>(0xC20D008));
	Sys::stdout->write("DRMDMA_RB_WPTR = 0x%08X\n", physmem->read<uint32_t>(0xC20D00C));
	Sys::stdout->write("SCRATCH_REG0 = 0x%08X\n", physmem->read<uint32_t>(0xC208500));
	Sys::stdout->write("SCRATCH_REG1 = 0x%08X\n", physmem->read<uint32_t>(0xC208504));
	Sys::stdout->write("SCRATCH_REG2 = 0x%08X\n", physmem->read<uint32_t>(0xC208508));
	Sys::stdout->write("SCRATCH_REG3 = 0x%08X\n", physmem->read<uint32_t>(0xC20850C));
	Sys::stdout->write("SCRATCH_REG4 = 0x%08X\n", physmem->read<uint32_t>(0xC208510));
	Sys::stdout->write("SCRATCH_REG5 = 0x%08X\n", physmem->read<uint32_t>(0xC208514));
	Sys::stdout->write("SCRATCH_REG6 = 0x%08X\n", physmem->read<uint32_t>(0xC208518));
	Sys::stdout->write("SCRATCH_REG7 = 0x%08X\n", physmem->read<uint32_t>(0xC20851C));
	Sys::stdout->write("SCRATCH_UMSK = 0x%08X\n", physmem->read<uint32_t>(0xC208540));
	Sys::stdout->write("SCRATCH_ADDR = 0x%08X\n", physmem->read<uint32_t>(0xC208544));
	Sys::stdout->write("DC0_INTERRUPT_CONTROL = 0x%08X\n", physmem->read<uint32_t>(0xC2060DC));
	Sys::stdout->write("DC1_INTERRUPT_CONTROL = 0x%08X\n", physmem->read<uint32_t>(0xC2068DC));
}

void Debugger::volumes(ArgParser *args) {
	if (!args->finish()) return;
	armdebug->printVolumes();
}

void Debugger::fileclients(ArgParser *args) {
	if (!args->finish()) return;
	armdebug->printFileClients();
}

void Debugger::slccache(ArgParser *args) {
	if (!args->finish()) return;
	armdebug->printSlcCacheState();
}
