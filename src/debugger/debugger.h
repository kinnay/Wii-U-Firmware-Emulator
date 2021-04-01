
#pragma once

#include "common/refcountedobj.h"
#include "common/sys.h"

#include "debugger/interface.h"
#include "debugger/expression.h"
#include "debugger/ppc.h"
#include "debugger/arm.h"
#include "debugger/dsp.h"

#include "physicalmemory.h"
#include "config.h"

#include <string>
#include <vector>
#include <map>


class Emulator;


class ArgParser {
public:
	ArgParser(Ref<EvalContext> context, const std::vector<std::string> &args);
	
	bool eof();
	bool finish();
	bool check();
	
	bool string(std::string *value);
	bool integer(uint32_t *value);
	
private:
	Ref<EvalContext> context;
	std::vector<std::string> args;
	
	int offset;
};


class Debugger : public RefCountedObj {
public:
	Debugger(Emulator *emulator);
	
	void show(int core);
	
private:
	void processCommand(std::string command, ArgParser *args);
	
	Ref<DebugInterface> getInterface();
	
	bool isPPC();
	bool isDSP();
	
	void help(ArgParser *parser);
	void quit(ArgParser *parser);
	void select(ArgParser *parser);
	
	void step(ArgParser *parser);
	void run(ArgParser *parser);
	void reset(ArgParser *parser);
	void restart(ArgParser *parser);
	#if STATS
	void stats(ArgParser *parser);
	#endif
	#if METRICS
	void metrics(ArgParser *parser);
	void syscalls(ArgParser *parser);
	void printSyscalls(int core);
	#endif
	
	#if BREAKPOINTS
	void breakp(ArgParser *parser);
	#endif
	#if WATCHPOINTS
	void watch(ArgParser *parser);
	#endif
	
	void state(ArgParser *parser);
	void print(ArgParser *parser);
	void trace(ArgParser *parser);
	
	void read(ArgParser *parser);
	void translate(ArgParser *parser);
	void memmap(ArgParser *parser);
	
	void modules(ArgParser *parser);
	void module(ArgParser *parser);
	void threads(ArgParser *parser);
	void thread(ArgParser *parser);
	void queues(ArgParser *parser);
	void queue(ArgParser *parser);
	void devices(ArgParser *parser);
	void ipc(ArgParser *parser);
	void hardware(ArgParser *parser);
	
	void volumes(ArgParser *parser);
	void fileclients(ArgParser *parser);
	void slccache(ArgParser *parser);
	
	Emulator *emulator;
	PhysicalMemory *physmem;
	
	Ref<DSPDebugger> dsp;
	Ref<ARMDebugger> arm;
	Ref<PPCDebugger> ppc[3];
	
	int core;
	bool debugging;
};
