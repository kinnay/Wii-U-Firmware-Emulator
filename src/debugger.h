
#pragma once

#include "armprocessor.h"
#include "ppcprocessor.h"
#include "expression.h"
#include "physicalmemory.h"
#include "processor.h"
#include "config.h"
#include "common/refcountedobj.h"
#include "common/sys.h"
#include <string>
#include <vector>
#include <map>


class ICommand;


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


class MemoryRegion : public RefCountedObj {
public:
	enum Access {
		NoAccess, ReadOnly, ReadWrite
	};
	
	MemoryRegion();
	
	bool check();
	bool overlaps(Ref<MemoryRegion> region);
	bool hits(Ref<MemoryRegion> region);
	void merge(Ref<MemoryRegion> region);
	
	static bool compare(Ref<MemoryRegion> a, Ref<MemoryRegion> b);
	
	uint64_t virt;
	uint64_t phys;
	uint32_t size;
	Access user;
	Access system;
	bool executable;
};

class MemoryMap {
public:
	MemoryMap(bool nx);
	
	void add(Ref<MemoryRegion> region);
	void print();
	
private:
	Ref<MemoryRegion> find(Ref<MemoryRegion> region);
	
	std::vector<Ref<MemoryRegion>> regions;
	bool nx;
};


class DebugInterface : public RefCountedObj {
public:
	virtual uint32_t pc() = 0;
	virtual std::string name() = 0;
	virtual Ref<EvalContext> getContext() = 0;
	virtual Processor *getProcessor() = 0;
	virtual bool translate(uint32_t *address) = 0;
	
	virtual void step() = 0;
	
	virtual void printState() = 0;
	virtual void printStateDetailed() = 0;
	virtual void printStackTrace() = 0;
	virtual void printMemoryMap() = 0;
	virtual void printThreads() = 0;
	virtual void printThreadDetails(uint32_t id) = 0;
	virtual void printIPCDriver() = 0;
};


class ARMDebugger : public DebugInterface {
public:
	ARMDebugger(PhysicalMemory *physmem, ARMProcessor *cpu);
	
	uint32_t pc();
	std::string name();
	Ref<EvalContext> getContext();
	Processor *getProcessor();
	bool translate(uint32_t *address);
	
	void step();
	
	void printState();
	void printStateDetailed();
	void printStackTrace();
	void printMemoryMap();
	void printThreads();
	void printThreadDetails(uint32_t id);
	void printMessageQueues();
	void printMessageQueueDetails(uint32_t id);
	void printDevices();
	void printVolumes();
	void printFileClients();
	void printSlcCacheState();
	void printIPCDriver();
	
private:
	std::string getQueueSuffix(uint32_t id);
	
	PhysicalMemory *physmem;
	ARMProcessor *cpu;
};


struct PPCModule : public RefCountedObj {
	static bool compareText(Ref<PPCModule> a, Ref<PPCModule> b);
	
	std::string path;
	std::string name;
	uint64_t text;
	uint64_t textsize;
	uint64_t data;
	uint64_t datasize;
};

class PPCDebugger : public DebugInterface {
public:
	typedef std::vector<Ref<PPCModule>> ModuleList;

	PPCDebugger(PhysicalMemory *physmem, PPCProcessor *cpu);
	
	uint32_t pc();
	std::string name();
	Ref<EvalContext> getContext();
	Processor *getProcessor();
	bool translate(uint32_t *address);
	
	void step();
	
	void printState();
	void printStateDetailed();
	void printStackTrace();
	void printMemoryMap();
	void printIPCDriver();
	void printModules();
	void printModuleDetails(std::string name);
	void printThreads();
	void printThreadDetails(uint32_t thread);
	
private:
	void printBAT(uint32_t batu, uint32_t batl);
	void printIPCRequest(uint32_t context);
	void printStackTraceForThread(uint32_t sp, ModuleList &modules);
	void printThreadWait(uint32_t thread);
	
	std::string formatAddress(uint32_t addr, ModuleList &modules);
	ModuleList getModules();
	
	PhysicalMemory *physmem;
	PPCProcessor *cpu;
};


class Debugger : public RefCountedObj {
public:
	Debugger(Emulator *emulator);
	
	void show(int core);
	
private:
	void processCommand(std::string command, ArgParser *args);
	
	Ref<DebugInterface> getCurrent();
	Processor *getProcessor();
	
	void help(ArgParser *parser);
	void quit(ArgParser *parser);
	void select(ArgParser *parser);
	
	void step(ArgParser *parser);
	void run(ArgParser *parser);
	void reset(ArgParser *parser);
	void restart(ArgParser *parser);
	#if STATS
	void stats(ArgParser *parser);
	void printPPCStats(int core);
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
	void ipcdriver(ArgParser *parser);
	void hardware(ArgParser *parser);
	
	void volumes(ArgParser *parser);
	void fileclients(ArgParser *parser);
	void slccache(ArgParser *parser);
	
	Emulator *emulator;
	PhysicalMemory *physmem;
	
	ARMProcessor *armcpu;
	PPCProcessor *ppccpu[3];
	
	Ref<ARMDebugger> armdebug;
	Ref<PPCDebugger> ppcdebug[3];
	
	int core;
	bool debugging;
};
