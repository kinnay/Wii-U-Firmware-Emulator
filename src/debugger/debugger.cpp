
#include "debugger/debugger.h"
#include "debugger/expression.h"
#include "debugger/common.h"
#include "emulator.h"

#include "common/sys.h"
#include "common/buffer.h"

#include <algorithm>


const char *HELP_TEXT =
	"General:\n"
	"    help\n"
	"    exit\n"
	"    quit\n"
	"    select arm/ppc0/ppc1/ppc2/dsp\n"
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
	"    read virt/phys <address> <length>  // ARM + PPC\n"
	"    read code/data <address> <length>  // DSP\n"
	"    translate <address>\n"
	"    memmap\n"
	"\n"
	"System state:\n"
	"    modules\n"
	"    module <name>\n"
	"    threads\n"
	"    thread <id>\n"
	"    hardware\n"
	"    ipc\n"
	"\n"
	"IOSU:\n"
	"    queues\n"
	"    queue <id>\n"
	"    devices\n"
	"    volumes\n"
	"    fileclients\n"
	"    slccache\n"
;


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
		Sys::out->write("Too many arguments.\n");
		return false;
	}
	return true;
}

bool ArgParser::check() {
	if (eof()) {
		Sys::out->write("Not enough arguments.\n");
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
		Sys::out->write("Failed to parse argument.\n");
		return false;
	}
	return true;
}


Debugger::Debugger(Emulator *emulator) {
	this->emulator = emulator;
	
	physmem = &emulator->physmem;
	
	arm = new ARMDebugger(physmem, &emulator->arm);
	ppc[0] = new PPCDebugger(physmem, &emulator->ppc[0], 0);
	ppc[1] = new PPCDebugger(physmem, &emulator->ppc[1], 1);
	ppc[2] = new PPCDebugger(physmem, &emulator->ppc[2], 2);
	dsp = new DSPDebugger(&emulator->dsp);
	
	core = 0;
}

Ref<DebugInterface> Debugger::getInterface() {
	if (core == 0) return arm;
	if (core == 1) return ppc[0];
	if (core == 2) return ppc[1];
	if (core == 3) return ppc[2];
	if (core == 4) return dsp;
	return nullptr;
}

bool Debugger::isPPC() { return core == 1 || core == 2 || core == 3; }
bool Debugger::isDSP() { return core == 4; }

void Debugger::show(int core) {
	emulator->pause();
	
	if (core != -1) {
		this->core = core;
	}
	
	debugging = true;
	while (debugging) {
		DebugInterface *debugger = getInterface();
		
		std::string format = StringUtils::format("%%s:%s> ", debugger->format());
		Sys::out->write(format, debugger->name(), debugger->pc());
		
		std::string line = Sys::in->readline();

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
	else if (command == "ipc") ipc(args);
	
	else if (command == "volumes") volumes(args);
	else if (command == "fileclients") fileclients(args);
	else if (command == "slccache") slccache(args);
	
	else {
		Sys::out->write("Unknown command.\n");
	}
}

void Debugger::help(ArgParser *args) {
	if (!args->finish()) return;
	Sys::out->write(HELP_TEXT);
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
	else if (name == "dsp") core = 4;
	else {
		Sys::out->write("Please provide a valid processor name.\n");
	}
}

void Debugger::step(ArgParser *args) {
	uint32_t count = 1;
	if (!args->eof()) {
		if (!args->integer(&count)) return;
		if (!args->finish()) return;
	}
	
	Processor *proc = getInterface()->getProcessor();
	if (!proc->isEnabled()) {
		Sys::out->write("Processor is disabled.\n");
		return;
	}
	
	for (uint32_t i = 0; i < count; i++) {
		proc->step();
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
	
	arm->printStats();
	for (int i = 0; i < 3; i++) {
		ppc[i]->printStats();
	}
	dsp->printStats();
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
		Sys::out->write("Please provide a valid sort order.\n");
		return;
	}
	
	if (name == "ppc0") ppccpu[0]->metrics.print(pmode);
	else if (name == "ppc1") ppccpu[1]->metrics.print(pmode);
	else if (name == "ppc2") ppccpu[2]->metrics.print(pmode);
	else {
		Sys::out->write("Please provide a valid processor name.\n");
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
		Sys::out->write("Please provide a valid processor name.\n");
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
	
	Sys::out->write("Syscalls executed: %i\n", total);
	Sys::out->write("\n");
	
	Sys::out->write("Sorted by frequency:\n");
	for (std::pair<uint32_t, uint64_t> instr : list) {
		Sys::out->write(
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
	
	DebugInterface *interface = getInterface();
	Processor *cpu = interface->getProcessor();
	if (command == "list") {
		if (!args->finish()) return;
		
		std::vector<uint32_t> breakpoints = cpu->breakpoints;
		std::sort(breakpoints.begin(), breakpoints.end());
		
		if (breakpoints.size() == 0) {
			Sys::out->write("No breakpoints are installed.\n");
		}
		else {
			if (breakpoints.size() == 1) {
				Sys::out->write("1 breakpoint:\n");
			}
			else {
				Sys::out->write("%i breakpoints:\n", breakpoints.size());
			}
			for (uint32_t bp : breakpoints) {
				std::string format = StringUtils::format("    0x%s\n", interface->format());
				Sys::out->write(format, bp);
			}
		}
	}
	else if (command == "clear") {
		if (!args->finish()) return;
		
		if (isPPC()) {
			for (int i = 0; i < 3; i++) {
				ppc[i]->getProcessor()->breakpoints.clear();
			}
		}
		else {
			cpu->breakpoints.clear();
		}
	}
	else if (command == "add" || command == "del") {
		uint32_t address;
		if (!args->integer(&address)) return;
		if (!args->finish()) return;
		
		bool exists = cpu->isBreakpoint(address);
		if (command == "add") {
			if (exists) {
				Sys::out->write("Breakpoint at 0x%X already exists.\n", address);
			}
			else {
				Sys::out->write("Added breakpoint at 0x%X.\n", address);
				if (isPPC()) {
					for (int i = 0; i < 3; i++) {
						ppc[i]->getProcessor()->addBreakpoint(address);
					}
				}
				else {
					cpu->addBreakpoint(address);
				}
			}
		}
		else {
			if (!exists) {
				Sys::out->write("Breakpoint at 0x%X does not exists.\n", address);
			}
			else {
				Sys::out->write("Removed breakpoint at 0x%X.\n", address);
				if (isPPC()) {
					for (int i = 0; i < 3; i++) {
						ppc[i]->getProcessor()->removeBreakpoint(address);
					}
				}
				else {
					cpu->removeBreakpoint(address);
				}
			}
		}
	}
	else {
		Sys::out->write("Unknown breakpoint command: %s\n", command);
	}
}
#endif

#if WATCHPOINTS
void Debugger::watch(ArgParser *args) {
	std::string command;
	if (!args->string(&command)) return;
	
	DebugInterface *interface = getInterface();
	Processor *cpu = interface->getProcessor();
	if (command == "list") {
		if (!args->finish()) return;
		
		size_t total = 0;
		for (int i = 0; i < 4; i++) {
			total += cpu->watchpoints[i / 2][i % 2].size();
		}
		
		if (total == 0) {
			Sys::out->write("No watchpoints are installed.\n");
		}
		else {
			if (total == 1) {
				Sys::out->write("1 watchpoint:\n");
			}
			else {
				Sys::out->write("%i watchpoints:\n", total);
			}
			
			for (int virt = 0; virt < 2; virt++) {
				for (int write = 0; write < 2; write++) {
					for (uint32_t wp : cpu->watchpoints[write][virt]) {
						std::string format = StringUtils::format("    0x%s (%%s, %%s)\n", interface->format());
						Sys::out->write(
							format, wp,
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
			Sys::out->write("Please specify either 'phys' or 'virt'.\n");
			return;
		}
		
		if (type != "read" && type != "write") {
			Sys::out->write("Please specify either 'read' or 'write'.\n");
			return;
		}
		
		bool write = type == "write";
		bool virt = mode == "virt";
		
		if (virt && isDSP()) {
			Sys::out->write("DSP does not have virtual memory.\n");
			return;
		}
		
		mode = virt ? "virtual" : "physical";
		
		bool exists = cpu->isWatchpoint(write, virt, address, 1);
		if (command == "add") {
			if (exists) {
				Sys::out->write("Watchpoint (%s) at %s address 0x%X already exists.\n", type, mode, address);
			}
			else {
				Sys::out->write("Added watchpoint (%s) at %s address 0x%X.\n", type, mode, address);
				cpu->addWatchpoint(write, virt, address);
			}
		}
		else {
			if (!exists) {
				Sys::out->write("Watchpoint (%s) at %s address 0x%X does not exists.\n", type, mode, address);
			}
			else {
				Sys::out->write("Removed watchpoint (%s) at %s address 0x%X.\n", type, mode, address);
				cpu->removeWatchpoint(write, virt, address);
			}
		}
	}
	else {
		Sys::out->write("Unknown watchpoint command: %s\n", command);
	}
}
#endif

void Debugger::state(ArgParser *args) {
	if (!args->eof()) {
		std::string param;
		if (!args->string(&param)) return;
		if (!args->finish()) return;
		
		if (param != "full") {
			Sys::out->write("Invalid argument.\n");
			return;
		}
		
		getInterface()->printStateDetailed();
	}
	else {
		getInterface()->printState();
	}
}

void Debugger::print(ArgParser *args) {
	uint32_t value;
	if (!args->integer(&value)) return;
	if (!args->finish()) return;
	
	Sys::out->write("0x%X (%i)\n", value, value);
}

void Debugger::trace(ArgParser *args) {
	if (!args->finish()) return;
	getInterface()->printStackTrace();
}

void Debugger::read(ArgParser *args) {
	std::string mode;
	uint32_t address, length;
	if (!args->string(&mode)) return;
	if (!args->integer(&address)) return;
	if (!args->integer(&length)) return;
	if (!args->finish()) return;
	
	if (mode != "phys" && mode != "virt" && mode != "code" && mode != "data") {
		Sys::out->write("Please specify either 'phys', 'virt', 'code' or 'data'.\n");
		return;
	}
	
	Buffer data;
	if (mode == "code" || mode == "data") {
		data = dsp->read(address, length, mode == "code");
	}
	else {
		DebugInterface *interface = getInterface();
		if (mode == "virt") {
			if (!interface->translate(&address)) {
				Sys::out->write("Address translation failed.\n");
				return;
			}
		}
		
		if (address + length < address) {
			length = ~address + 1;
		}
		
		data = physmem->read(address, length);
	}
	
	Sys::out->write("%s\n", data.hexstring());
	
	if (mode == "phys" || mode == "virt") {
		std::string text = data.tostring();
		for (int i = 0; i < text.size(); i++) {
			if (!StringUtils::is_printable(text[i])) {
				text[i] = ' ';
			}
		}
		Sys::out->write("\n%s\n", text);
	}
}

void Debugger::translate(ArgParser *args) {
	uint32_t addr;
	if (!args->integer(&addr)) return;
	if (!args->finish()) return;
	
	if (getInterface()->translate(&addr)) {
		std::string format = StringUtils::format("0x%s\n", getInterface()->format());
		Sys::out->write(format, addr);
	}
	else {
		Sys::out->write("Address translation failed.\n");
	}
}

void Debugger::memmap(ArgParser *args) {
	if (!args->finish()) return;
	getInterface()->printMemoryMap();
}

void Debugger::modules(ArgParser *args) {
	if (!args->finish()) return;
	ppc[1]->printModules();
}

void Debugger::module(ArgParser *args) {
	std::string name;
	if (!args->string(&name)) return;
	if (!args->finish()) return;
	ppc[1]->printModuleDetails(name);
}

void Debugger::threads(ArgParser *args) {
	if (!args->finish()) return;
	getInterface()->printThreads();
}

void Debugger::thread(ArgParser *args) {
	uint32_t tid;
	if (!args->integer(&tid)) return;
	if (!args->finish()) return;
	getInterface()->printThreadDetails(tid);
}

void Debugger::queues(ArgParser *args) {
	if (!args->finish()) return;
	arm->printMessageQueues();
}

void Debugger::queue(ArgParser *args) {
	uint32_t id;
	if (!args->integer(&id)) return;
	if (!args->finish()) return;
	arm->printMessageQueueDetails(id);
}

void Debugger::devices(ArgParser *args) {
	if (!args->finish()) return;
	arm->printDevices();
}

void Debugger::ipc(ArgParser *args) {
	if (!args->finish()) return;
	
	for (int i = 0; i < 3; i++) {
		if (i != 0) {
			Sys::out->write("\n");
		}
		ppc[i]->printIPC();
	}
}

void Debugger::hardware(ArgParser *args) {
	if (!args->finish()) return;
	Sys::out->write("PI_CPU0_INTMR = 0x%08X\n", physmem->read<uint32_t>(0xC000078));
	Sys::out->write("PI_CPU0_INTSR = 0x%08X\n", physmem->read<uint32_t>(0xC00007C)); 
	Sys::out->write("PI_CPU1_INTMR = 0x%08X\n", physmem->read<uint32_t>(0xC000080));
	Sys::out->write("PI_CPU1_INTSR = 0x%08X\n", physmem->read<uint32_t>(0xC000084));
	Sys::out->write("PI_CPU2_INTMR = 0x%08X\n", physmem->read<uint32_t>(0xC000088));
	Sys::out->write("PI_CPU2_INTSR = 0x%08X\n", physmem->read<uint32_t>(0xC00008C));
	Sys::out->write("\n");
	Sys::out->write("WG_CPU0_BASE = 0x%08X\n", physmem->read<uint32_t>(0xC000040));
	Sys::out->write("WG_CPU0_TOP = 0x%08X\n", physmem->read<uint32_t>(0xC000044));
	Sys::out->write("WG_CPU0_PTR = 0x%08X\n", physmem->read<uint32_t>(0xC000048));
	Sys::out->write("WG_CPU0_THRESHOLD = 0x%08X\n", physmem->read<uint32_t>(0xC00004C));
	Sys::out->write("WG_CPU1_BASE = 0x%08X\n", physmem->read<uint32_t>(0xC000050));
	Sys::out->write("WG_CPU1_TOP = 0x%08X\n", physmem->read<uint32_t>(0xC000054));
	Sys::out->write("WG_CPU1_PTR = 0x%08X\n", physmem->read<uint32_t>(0xC000058));
	Sys::out->write("WG_CPU1_THRESHOLD = 0x%08X\n", physmem->read<uint32_t>(0xC00005C));
	Sys::out->write("WG_CPU2_BASE = 0x%08X\n", physmem->read<uint32_t>(0xC000060));
	Sys::out->write("WG_CPU2_TOP = 0x%08X\n", physmem->read<uint32_t>(0xC000064));
	Sys::out->write("WG_CPU2_PTR = 0x%08X\n", physmem->read<uint32_t>(0xC000068));
	Sys::out->write("WG_CPU2_THRESHOLD = 0x%08X\n", physmem->read<uint32_t>(0xC00006C));
	Sys::out->write("\n");
	Sys::out->write("LT_ARM_INTMR_ALL = 0x%08X\n", physmem->read<uint32_t>(0xD000478));
	Sys::out->write("LT_ARM_INTSR_ALL = 0x%08X\n", physmem->read<uint32_t>(0xD000470));
	Sys::out->write("LT_ARM_INTMR_LT = 0x%08X\n", physmem->read<uint32_t>(0xD00047C));
	Sys::out->write("LT_ARM_INTSR_LT = 0x%08X\n", physmem->read<uint32_t>(0xD000474));
	Sys::out->write("LT_PPC0_INTMR_ALL = 0x%08X\n", physmem->read<uint32_t>(0xD000448));
	Sys::out->write("LT_PPC0_INTSR_ALL = 0x%08X\n", physmem->read<uint32_t>(0xD000440));
	Sys::out->write("LT_PPC0_INTMR_LT = 0x%08X\n", physmem->read<uint32_t>(0xD00044C));
	Sys::out->write("LT_PPC0_INTSR_LT = 0x%08X\n", physmem->read<uint32_t>(0xD000444));
	Sys::out->write("LT_PPC1_INTMR_ALL = 0x%08X\n", physmem->read<uint32_t>(0xD000458));
	Sys::out->write("LT_PPC1_INTSR_ALL = 0x%08X\n", physmem->read<uint32_t>(0xD000450));
	Sys::out->write("LT_PPC1_INTMR_LT = 0x%08X\n", physmem->read<uint32_t>(0xD00045C));
	Sys::out->write("LT_PPC1_INTSR_LT = 0x%08X\n", physmem->read<uint32_t>(0xD000454));
	Sys::out->write("LT_PPC2_INTMR_ALL = 0x%08X\n", physmem->read<uint32_t>(0xD000468));
	Sys::out->write("LT_PPC2_INTSR_ALL = 0x%08X\n", physmem->read<uint32_t>(0xD000460));
	Sys::out->write("LT_PPC2_INTMR_LT = 0x%08X\n", physmem->read<uint32_t>(0xD00046C));
	Sys::out->write("LT_PPC2_INTSR_LT = 0x%08X\n", physmem->read<uint32_t>(0xD000464));
	Sys::out->write("\n");
	Sys::out->write("IH_RB_BASE = 0x%08X\n", physmem->read<uint32_t>(0xC203E04));
	Sys::out->write("IH_RB_RPTR = 0x%08X\n", physmem->read<uint32_t>(0xC203E08));
	Sys::out->write("IH_RB_WPTR_ADDR_LO = 0x%08X\n", physmem->read<uint32_t>(0xC203E14));
	Sys::out->write("CP_RB_BASE = 0x%08X\n", physmem->read<uint32_t>(0xC20C100));
	Sys::out->write("CP_RB_RPTR_ADDR = 0x%08X\n", physmem->read<uint32_t>(0xC20C10C));
	Sys::out->write("CP_RB_WPTR = 0x%08X\n", physmem->read<uint32_t>(0xC20C114));
	Sys::out->write("DRMDMA_RB_CNTL = 0x%08X\n", physmem->read<uint32_t>(0xC20D000));
	Sys::out->write("DRMDMA_RB_BASE = 0x%08X\n", physmem->read<uint32_t>(0xC20D004));
	Sys::out->write("DRMDMA_RB_RPTR = 0x%08X\n", physmem->read<uint32_t>(0xC20D008));
	Sys::out->write("DRMDMA_RB_WPTR = 0x%08X\n", physmem->read<uint32_t>(0xC20D00C));
	Sys::out->write("SCRATCH_REG0 = 0x%08X\n", physmem->read<uint32_t>(0xC208500));
	Sys::out->write("SCRATCH_REG1 = 0x%08X\n", physmem->read<uint32_t>(0xC208504));
	Sys::out->write("SCRATCH_REG2 = 0x%08X\n", physmem->read<uint32_t>(0xC208508));
	Sys::out->write("SCRATCH_REG3 = 0x%08X\n", physmem->read<uint32_t>(0xC20850C));
	Sys::out->write("SCRATCH_REG4 = 0x%08X\n", physmem->read<uint32_t>(0xC208510));
	Sys::out->write("SCRATCH_REG5 = 0x%08X\n", physmem->read<uint32_t>(0xC208514));
	Sys::out->write("SCRATCH_REG6 = 0x%08X\n", physmem->read<uint32_t>(0xC208518));
	Sys::out->write("SCRATCH_REG7 = 0x%08X\n", physmem->read<uint32_t>(0xC20851C));
	Sys::out->write("SCRATCH_UMSK = 0x%08X\n", physmem->read<uint32_t>(0xC208540));
	Sys::out->write("SCRATCH_ADDR = 0x%08X\n", physmem->read<uint32_t>(0xC208544));
	Sys::out->write("D1CRTC_INTERRUPT_CONTROL = 0x%08X\n", physmem->read<uint32_t>(0xC2060DC));
	Sys::out->write("D1GRPH_ENABLE = 0x%08X\n", physmem->read<uint32_t>(0xC206100));
	Sys::out->write("D1GRPH_CONTROL = 0x%08X\n", physmem->read<uint32_t>(0xC206104));
	Sys::out->write("D1GRPH_PRIMARY_SURFACE_ADDRESS = 0x%08X\n", physmem->read<uint32_t>(0xC206110));
	Sys::out->write("D1GRPH_SECONDARY_SURFACE_ADDRESS = 0x%08X\n", physmem->read<uint32_t>(0xC206118));
	Sys::out->write("D1OVL_ENABLE = 0x%08X\n", physmem->read<uint32_t>(0xC206180));
	Sys::out->write("D1OVL_CONTROL = 0x%08X\n", physmem->read<uint32_t>(0xC206184));
	Sys::out->write("D1OVL_SURFACE_ADDRESS = 0x%08X\n", physmem->read<uint32_t>(0xC206190));
	Sys::out->write("D2CRTC_INTERRUPT_CONTROL = 0x%08X\n", physmem->read<uint32_t>(0xC2068DC));
	Sys::out->write("D2GRPH_ENABLE = 0x%08X\n", physmem->read<uint32_t>(0xC206900));
	Sys::out->write("D2GRPH_CONTROL = 0x%08X\n", physmem->read<uint32_t>(0xC206904));
	Sys::out->write("D2GRPH_PRIMARY_SURFACE_ADDRESS = 0x%08X\n", physmem->read<uint32_t>(0xC206910));
	Sys::out->write("D2GRPH_SECONDARY_SURFACE_ADDRESS = 0x%08X\n", physmem->read<uint32_t>(0xC206918));
	Sys::out->write("D2OVL_ENABLE = 0x%08X\n", physmem->read<uint32_t>(0xC206980));
	Sys::out->write("D2OVL_CONTROL = 0x%08X\n", physmem->read<uint32_t>(0xC206984));
	Sys::out->write("D2OVL_SURFACE_ADDRESS = 0x%08X\n", physmem->read<uint32_t>(0xC206990));
}

void Debugger::volumes(ArgParser *args) {
	if (!args->finish()) return;
	arm->printVolumes();
}

void Debugger::fileclients(ArgParser *args) {
	if (!args->finish()) return;
	arm->printFileClients();
}

void Debugger::slccache(ArgParser *args) {
	if (!args->finish()) return;
	arm->printSlcCacheState();
}
