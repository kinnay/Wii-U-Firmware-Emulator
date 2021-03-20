
#include "debugger/ppc.h"
#include "debugger/memorymap.h"
#include "debugger/common.h"

#include <algorithm>


static uint32_t phys(uint32_t address) {
	if (address >= 0xFFE00000) {
		return address - 0xFFE00000 + 0x08000000;
	}
	return address;
}


bool PPCModule::compareText(Ref<PPCModule> a, Ref<PPCModule> b) {
	return a->text < b->text;
}


PPCDebugger::PPCDebugger(PhysicalMemory *physmem, PPCProcessor *cpu, int index) {
	this->physmem = physmem;
	this->index = index;
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
	Sys::out->write(
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
	Sys::out->write(
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
	Sys::out->write("\n");
	Sys::out->write(
		"f0  = %08X:%08X f1  = %08X:%08X f2  = %08X:%08X\n"
		"f3  = %08X:%08X f4  = %08X:%08X f5  = %08X:%08X\n"
		"f6  = %08X:%08X f7  = %08X:%08X f8  = %08X:%08X\n"
		"f9  = %08X:%08X f10 = %08X:%08X f11 = %08X:%08X\n"
		"f12 = %08X:%08X f13 = %08X:%08X f14 = %08X:%08X\n"
		"f15 = %08X:%08X f16 = %08X:%08X f17 = %08X:%08X\n"
		"f18 = %08X:%08X f19 = %08X:%08X f20 = %08X:%08X\n"
		"f21 = %08X:%08X f22 = %08X:%08X f23 = %08X:%08X\n"
		"f24 = %08X:%08X f25 = %08X:%08X f26 = %08X:%08X\n"
		"f27 = %08X:%08X f28 = %08X:%08X f29 = %08X:%08X\n"
		"f30 = %08X:%08X f31 = %08X:%08X\n",
		cpu->core.fprs[0].uw0, cpu->core.fprs[0].uw1,
		cpu->core.fprs[1].uw0, cpu->core.fprs[1].uw1,
		cpu->core.fprs[2].uw0, cpu->core.fprs[2].uw1,
		cpu->core.fprs[3].uw0, cpu->core.fprs[3].uw1,
		cpu->core.fprs[4].uw0, cpu->core.fprs[4].uw1,
		cpu->core.fprs[5].uw0, cpu->core.fprs[5].uw1,
		cpu->core.fprs[6].uw0, cpu->core.fprs[6].uw1,
		cpu->core.fprs[7].uw0, cpu->core.fprs[7].uw1,
		cpu->core.fprs[8].uw0, cpu->core.fprs[8].uw1,
		cpu->core.fprs[9].uw0, cpu->core.fprs[9].uw1,
		cpu->core.fprs[10].uw0, cpu->core.fprs[10].uw1,
		cpu->core.fprs[11].uw0, cpu->core.fprs[11].uw1,
		cpu->core.fprs[12].uw0, cpu->core.fprs[12].uw1,
		cpu->core.fprs[13].uw0, cpu->core.fprs[13].uw1,
		cpu->core.fprs[14].uw0, cpu->core.fprs[14].uw1,
		cpu->core.fprs[15].uw0, cpu->core.fprs[15].uw1,
		cpu->core.fprs[16].uw0, cpu->core.fprs[16].uw1,
		cpu->core.fprs[17].uw0, cpu->core.fprs[17].uw1,
		cpu->core.fprs[18].uw0, cpu->core.fprs[18].uw1,
		cpu->core.fprs[19].uw0, cpu->core.fprs[19].uw1,
		cpu->core.fprs[20].uw0, cpu->core.fprs[20].uw1,
		cpu->core.fprs[21].uw0, cpu->core.fprs[21].uw1,
		cpu->core.fprs[22].uw0, cpu->core.fprs[22].uw1,
		cpu->core.fprs[23].uw0, cpu->core.fprs[23].uw1,
		cpu->core.fprs[24].uw0, cpu->core.fprs[24].uw1,
		cpu->core.fprs[25].uw0, cpu->core.fprs[25].uw1,
		cpu->core.fprs[26].uw0, cpu->core.fprs[26].uw1,
		cpu->core.fprs[27].uw0, cpu->core.fprs[27].uw1,
		cpu->core.fprs[28].uw0, cpu->core.fprs[28].uw1,
		cpu->core.fprs[29].uw0, cpu->core.fprs[29].uw1,
		cpu->core.fprs[30].uw0, cpu->core.fprs[30].uw1,
		cpu->core.fprs[31].uw0, cpu->core.fprs[31].uw1
	);
	Sys::out->write("\n");
	Sys::out->write(
		"srr0 = %08X srr1 = %08X dar  = %08X dsisr= %08X\n"
		"msr  = %08X dec  = %08X dabr = %08X iabr = %08X\n",
		cpu->core.sprs[PPCCore::SRR0], cpu->core.sprs[PPCCore::SRR1],
		cpu->core.sprs[PPCCore::DAR], cpu->core.sprs[PPCCore::DSISR],
		cpu->core.msr, cpu->core.sprs[PPCCore::DEC],
		cpu->core.sprs[PPCCore::DABR], cpu->core.sprs[PPCCore::IABR]
	);
	Sys::out->write("\n");
	Sys::out->write(
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

	Sys::out->write("\n");
	Sys::out->write("pc = %s\n", formatAddress(pc, modules));
	Sys::out->write("lr = %s\n", formatAddress(lr, modules));
	Sys::out->write("\n");
	
	translate(&sp);
	sp = physmem->read<uint32_t>(sp);
	
	printStackTraceForThread(sp, modules);
}

void PPCDebugger::printStackTraceForThread(uint32_t sp, ModuleList &modules) {
	Sys::out->write("Stack trace:\n");
	while (sp) {
		translate(&sp);
		
		uint32_t lr = physmem->read<uint32_t>(sp + 4);
		if (!lr) break;
		
		Sys::out->write("    %s\n", formatAddress(lr, modules));

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
	Sys::out->write("DBAT:\n");
	for (int i = 0; i < 8; i++) {
		uint32_t batu = cpu->core.sprs[PPCCore::DBAT0U + i * 2 + i / 4 * 8];
		uint32_t batl = cpu->core.sprs[PPCCore::DBAT0L + i * 2 + i / 4 * 8];
		Sys::out->write("    dbat%i: ", i);
		printBAT(batu, batl);
	}
	
	Sys::out->write("\nIBAT:\n");
	for (int i = 0; i < 8; i++) {
		uint32_t batu = cpu->core.sprs[PPCCore::IBAT0U + i * 2 + i / 4 * 8];
		uint32_t batl = cpu->core.sprs[PPCCore::IBAT0L + i * 2 + i / 4 * 8];
		Sys::out->write("    ibat%i: ", i);
		printBAT(batu, batl);
	}
	
	uint32_t sdr1 = cpu->core.sprs[PPCCore::SDR1];
	uint32_t pagetbl = phys(sdr1 & 0xFFFF0000);
	uint32_t pagemask = sdr1 & 0x1FF;
	uint32_t hashmask = (pagemask << 10) | 0x3FF;
	
	Sys::out->write("\nPage table (%08X):\n", pagetbl);
	
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
		
		Sys::out->write(
			"%08X-%08X => %08X-%08X (%s, %s, %s)\n",
			vaddr, vaddr + size, paddr, paddr + size,
			caching, access, validity
		);
	}
	else {
		Sys::out->write("disabled\n");
	}
}

void PPCDebugger::printModules() {
	ModuleList modules = getModules();
	std::sort(modules.begin(), modules.end(), PPCModule::compareText);
	
	for (Ref<PPCModule> module : modules) {
		Sys::out->write(" %08X: %s\n", module->text, module->path);
	}
}

void PPCDebugger::printModuleDetails(std::string name) {
	ModuleList modules = getModules();
	for (Ref<PPCModule> module : modules) {
		if (module->path.find(name) != std::string::npos) {
			Sys::out->write("%s:\n", module->path);
			Sys::out->write("    .text: %08X - %08X\n", module->text, module->text + module->textsize);
			Sys::out->write("    .data: %08X - %08X\n", module->data, module->data + module->datasize);
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
		
		Sys::out->write(" %08X:  %-12s  %-10s %-9s  %s\n", addr, owner, core, state, name);
		
		thread = physmem->read<uint32_t>(thread + 0x38C);
	}
}

void PPCDebugger::printThreadDetails(uint32_t thread) {
	if (!(cpu->core.msr & 0x10)) {
		Sys::out->write("MMU is disabled.\n");
		return;
	}
	
	if (!translate(&thread)) {
		Sys::out->write("Address translation failed.\n");
		return;
	}
	
	uint32_t tag = physmem->read<uint32_t>(thread + 0x320);
	if (tag != 0x74487244) { //tHrD
		Sys::out->write("This is not a valid thread.\n");
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
	
	Sys::out->write("Name: %s\n\n", name);
	Sys::out->write("Entry point: %s\n\n", formatAddress(entryPoint, modules));
	Sys::out->write("Stack: 0x%08X - 0x%08X (0x%X bytes)\n\n", stackTop, stackBase, stackBase - stackTop);
	printStackTraceForThread(sp, modules);
	Sys::out->write("\n");
	
	uint8_t state = physmem->read<uint8_t>(thread + 0x324);
	if (state == 1) Sys::out->write("This thread is ready to run.\n");
	else if (state == 2) Sys::out->write("This thread is running right now.\n");
	else if (state == 4) printThreadWait(thread);
	else if (state == 8) Sys::out->write("This thread is dead.\n");
	else {
		Sys::out->write("This thread has an invalid state.\n");
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
			if (tag == 0x65566E54) Sys::out->write("This thread is waiting for an event: %s\n", name);
			else if (tag == 0x614C724D) Sys::out->write("This thread is waiting for an alarm: %s\n", name);
			else if (tag == 0x6D557458) Sys::out->write("This thread is waiting for a mutex: %s\n", name);
			else if (tag == 0x634E6456) Sys::out->write("This thread is waiting for a condition variable: %s\n", name);
			else if (tag == 0x73506852) Sys::out->write("This thread is waiting for a semaphore: %s\n", name);
			else if (tag == 0x6D536751) Sys::out->write("This thread is waiting for a message queue: %s\n", name);
			else if (tag == 0x614C6D51) Sys::out->write("This thread is waiting for an alarm queue: %s\n", name);
			else {
				Sys::out->write("This thread is currently waiting.\n");
			}
			return;
		}
	}
	Sys::out->write("This thread is currently waiting.\n");
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

void PPCDebugger::printIPC() {
	Sys::out->write("PPC %i:\n", index);
	
	uint32_t driver = phys(0xFFE86760 + index * 0x2178);
	
	uint32_t fifo = driver + 0x3E8;
	Sys::out->write("    Sending: %i\n", physmem->read<uint32_t>(fifo + 8));
	Sys::out->write("    Peak:    %i\n", physmem->read<uint32_t>(fifo + 12));
	
	Sys::out->write("\n");
	Sys::out->write("    Waiting for reply:\n");
	for (int i = 0; i < 0xB0; i++) {
		uint32_t addr = driver + 0x13B8 + i * 0x14;
		uint32_t callback = physmem->read<uint32_t>(addr + 4);
		if (callback) {
			uint32_t request = phys(physmem->read<uint32_t>(addr + 0x10));
			printIPCRequest(request);
		}
	}
}

void PPCDebugger::printIPCRequest(uint32_t request) {
	uint32_t cmd = physmem->read<uint32_t>(request);
	uint32_t arg0 = physmem->read<uint32_t>(request + 0x24);
	uint32_t arg1 = physmem->read<uint32_t>(request + 0x28);
	uint32_t arg2 = physmem->read<uint32_t>(request + 0x2C);
	uint32_t arg3 = physmem->read<uint32_t>(request + 0x30);
	uint32_t arg4 = physmem->read<uint32_t>(request + 0x34);
	if (cmd == 1) {
		std::string name = physmem->read<std::string>(arg0);
		Sys::out->write("        OPEN(%s)\n", name);
	}
	else if (cmd == 2) Sys::out->write("        CLOSE()\n");
	else if (cmd == 3) Sys::out->write("        READ(0x%X, 0x%X)\n", arg0, arg1);
	else if (cmd == 4) Sys::out->write("        WRITE(0xX, 0x%X)\n", arg0, arg1);
	else if (cmd == 5) Sys::out->write("        SEEK(0x%X, %i)\n", arg0, arg1);
	else if (cmd == 6) Sys::out->write("        IOCTL(%i, 0x%X, 0x%X, 0x%X, 0x%X)\n", arg0, arg1, arg2, arg3, arg4);
	else if (cmd == 7) Sys::out->write("        IOCTLV(%i, %i, %i, 0x%X)\n", arg0, arg1, arg2, arg3);
	else {
		Sys::out->write("        ?\n");
	}
}

#if STATS
void PPCDebugger::printStats() {
	Sys::out->write("\n");
	Sys::out->write("PPC %i:\n", index);
	Sys::out->write("    Instructions executed:       %i\n", cpu->instrsExecuted);
	Sys::out->write(
		"    Instructions executed (jit): %i (%i%%)\n", cpu->jit.instrsExecuted,
		percentage(cpu->jit.instrsExecuted, cpu->instrsExecuted)
	);
	Sys::out->write(
		"    Instructions compiled (jit): %i (avg usage: %i)\n", cpu->jit.instrsCompiled,
		divide(cpu->jit.instrsExecuted, cpu->jit.instrsCompiled)
	);
	Sys::out->write(
		"    JIT memory usage:            %i bytes (%i per instr)\n", cpu->jit.instrSize,
		divide(cpu->jit.instrSize, cpu->jit.instrsCompiled)
	);
	Sys::out->write("    \n");
	Sys::out->write("    MMU cache hits:   %i\n", cpu->mmu.cache.hits);
	Sys::out->write("    MMU cache misses: %i\n", cpu->mmu.cache.misses);
	Sys::out->write("    \n");
	Sys::out->write("    DSI exceptions    %i\n", cpu->core.dsiExceptions);
	Sys::out->write("    ISI exceptions:   %i\n", cpu->core.isiExceptions);
	Sys::out->write("    External intr:    %i\n", cpu->core.externalInterrupts);
	Sys::out->write("    Decrementer intr: %i\n", cpu->core.decrementerInterrupts);
	Sys::out->write("    System calls:     %i\n", cpu->core.systemCalls);
}
#endif
