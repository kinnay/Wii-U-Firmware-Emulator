
#include "debugger/arm.h"
#include "debugger/common.h"
#include "debugger/memorymap.h"


static const char *ProcessNames[] = {
	"IOS-KERNEL",
	"IOS-MCP",
	"IOS-BSP",
	"IOS-CRYPTO",
	"IOS-USB",
	"IOS-FS",
	"IOS-PAD",
	"IOS-NET",
	"IOS-ACP",
	"IOS-NSEC",
	"IOS-AUXIL",
	"IOS-NIM-BOSS",
	"IOS-FPD",
	"IOS-TEST",
	
	"COS-KERNEL",
	"COS-ROOT",
	"COS-02",
	"COS-03",
	"COS-OVERLAY",
	"COS-HBM",
	"COS-ERROR",
	"COS-MASTER"
};


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
	Sys::out->write(
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
	
	Sys::out->write("Current mode: %s\n\n", getModeName(cpu->core.getMode()));
	Sys::out->write(
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
	Sys::out->write(
		"FIQ mode:\n"
		"    R8 = %08X R9 = %08X R10= %08X R11 = %08X\n"
		"    R12= %08X SP = %08X LR = %08X SPSR= %08X\n",
		cpu->core.regsFiq[0], cpu->core.regsFiq[1], cpu->core.regsFiq[2],
		cpu->core.regsFiq[3], cpu->core.regsFiq[4], cpu->core.regsFiq[5],
		cpu->core.regsFiq[6], cpu->core.spsrFiq
	);
	Sys::out->write(
		"IRQ mode:\n"
		"    SP = %08X LR = %08X SPSR = %08X\n",
		cpu->core.regsIrq[0], cpu->core.regsIrq[1], cpu->core.spsrIrq
	);
	Sys::out->write(
		"SVC mode:\n"
		"    SP = %08X LR = %08X SPSR = %08X\n",
		cpu->core.regsSvc[0], cpu->core.regsSvc[1], cpu->core.spsrSvc
	);
	Sys::out->write(
		"Abort mode:\n"
		"    SP = %08X LR = %08X SPSR = %08X\n",
		cpu->core.regsAbt[0], cpu->core.regsAbt[1], cpu->core.spsrAbt
	);
	Sys::out->write(
		"Undefined mode:\n"
		"    SP = %08X LR = %08X SPSR = %08X\n\n",
		cpu->core.regsUnd[0], cpu->core.regsUnd[1], cpu->core.spsrUnd
	);
	Sys::out->write(
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
	
	Sys::out->write("PC = 0x%08X  LR = 0x%08X  SP = 0x%08X\n", pc, lr, sp);
	
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
	
	Sys::out->write("Stack trace:\n");
	while (true) {
		if (pc == 0xD400200) break;
		
		if (thumb) {
			pc -= 2;
			
			uint16_t instr = physmem->read<uint16_t>(pc);
			if (instr == 0) {
				Sys::out->write("Stack trace may be unreliable.\n");
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
					Sys::out->write("    0x%08X (thumb)\n", pc);
				}
				else {
					thumb = false;
					Sys::out->write("    0x%08X (arm)\n", pc);
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
				Sys::out->write("Stack trace may be unreliable.\n");
				break;
			}
			
			if (instr == 0xE52DE004) { // STR LR, [SP,#-4]!
				pc = physmem->read<uint32_t>(sp);
				sp += 4;
				
				if (pc & 1) {
					thumb = true;
					pc &= ~1;
					Sys::out->write("    0x%08X (thumb)\n", pc);
				}
				else {
					Sys::out->write("    0x%08X (arm)\n", pc);
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
					Sys::out->write("    0x%08X (thumb)\n", pc);
				}
				else {
					Sys::out->write("    0x%08X (arm)\n", pc);
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
		Sys::out->write("MMU is disabled\n");
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
				Sys::out->write(
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
		Sys::out->write("Invalid thread id.\n");
		return;
	}
	
	uint32_t thread = 0xFFFF4D78 + id * 0xC8;
	uint32_t state = physmem->read<uint32_t>(thread + 0x50);
	
	if (state == 0) {
		Sys::out->write("Thread is not in use.\n");
		return;
	}
	
	if (state > 6) {
		Sys::out->write("Thread has invalid state.\n");
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
		Sys::out->write("Thread has invalid pid.\n");
		return;
	}
	
	Sys::out->write("Thread is owned by %s\n\n", ProcessNames[pid]);
	
	uint32_t priority = physmem->read<uint32_t>(thread + 0x4C);
	Sys::out->write("Priority: %i\n\n", priority);
	
	Sys::out->write(
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
	Sys::out->write("Stack: 0x%08X - 0x%08X (0x%X bytes)\n\n", stackTop - stackSize, stackTop, stackSize);
	
	if (state == 1) Sys::out->write("This thread is ready to run.\n");
	else if (state == 2) Sys::out->write("This thread is running right now.\n");
	else if (state == 3) Sys::out->write("This thread is suspended.\n");
	else if (state == 4) Sys::out->write("This thread is currently waiting.\n");
	else if (state == 5) Sys::out->write("This thread has been canceled.\n");
	else if (state == 6) Sys::out->write("This thread has triggered an exception.\n");
}

void ARMDebugger::printMessageQueues() {
	uint32_t queue = 0x8150C84;
	for (int i = 0; i < 750; i++) {
		uint32_t capacity = physmem->read<uint32_t>(queue + 0x10);
		uint32_t buffer = physmem->read<uint32_t>(queue + 0x14);
		uint8_t pid = physmem->read<uint8_t>(queue + 0x1C);
		if (capacity != 0 && pid < sizeof(ProcessNames) / sizeof(ProcessNames[0])) {
			Sys::out->write(
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
		Sys::out->write("Message queue is not in use.\n");
		return;
	}
	
	if (pid >= sizeof(ProcessNames) / sizeof(ProcessNames[0])) {
		Sys::out->write("Message queue has invalid pid.\n");
		return;
	}
	
	Sys::out->write(
		"Owner: %-12s Buffer: %08X  Capacity: %i\n",
		ProcessNames[pid], buffer, capacity
	);
	
	if (use & 1) {
		for (int i = 0; i < 48; i++) {
			uint32_t event = 0x81A3800 + i * 0x10;
			uint32_t handler = physmem->read<uint32_t>(event);
			uint32_t message = physmem->read<uint32_t>(event + 4);
			if (handler == queue) {
				Sys::out->write(
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
				Sys::out->write(
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
				Sys::out->write(" %3i:  %-8s  %s\n", i, device, cwd);
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
			
			Sys::out->write(" %2i:  %-10s %s\n", index, name, format);
			
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
			Sys::out->write("No cached areas found.\n");
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
				
				Sys::out->write(" %3i:  0x%X - 0x%X (%i blocks)\n", i, startBlock, endBlock, endBlock - startBlock);
				
				index = physmem->read<uint32_t>(addr);
			}
		}
	}
}

#if STATS
void ARMDebugger::printStats() {
	Sys::out->write("ARM:\n");
	Sys::out->write("    ARM instrs executed:       %i\n", cpu->armInstrs);
	Sys::out->write(
		"    ARM instrs executed (jit): %i (%i%%)\n",
		cpu->jit.instrsExecuted, percentage(cpu->jit.instrsExecuted, cpu->armInstrs)
	);
	Sys::out->write(
		"    ARM instrs compiled (jit): %i (avg usage: %i)\n",
		cpu->jit.instrsCompiled, divide(cpu->jit.instrsExecuted, cpu->jit.instrsCompiled)
	);
	Sys::out->write(
		"    ARM jit memory usage:      %i bytes (%i per instr)\n",
		cpu->jit.instrSize, divide(cpu->jit.instrSize, cpu->jit.instrsCompiled)
	);
	Sys::out->write("    \n");
	Sys::out->write(
		"    Thumb instrs executed:  %i\n",
		cpu->thumb.instrsExecuted
	);
	Sys::out->write(
		"    Thumb instrs compiled:  %i (avg usage: %i)\n",
		cpu->thumb.instrsCompiled, divide(cpu->thumb.instrsExecuted, cpu->thumb.instrsCompiled)
	);
	Sys::out->write(
		"    Thumb jit memory usage: %i bytes (%i per instr)\n",
		cpu->thumb.instrSize, divide(cpu->thumb.instrSize, cpu->thumb.instrsCompiled)
	);
	Sys::out->write("    \n");
	Sys::out->write("    Number of data reads:  %i\n", cpu->dataReads);
	Sys::out->write("    Number of data writes: %i\n", cpu->dataWrites);
	Sys::out->write("    \n");
	Sys::out->write("    MMU cache hits:   %i\n", cpu->mmu.cache.hits);
	Sys::out->write("    MMU cache misses: %i\n", cpu->mmu.cache.misses);
}
#endif
