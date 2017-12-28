
#pragma once

#include "class_interpreter.h"
#include "class_armcore.h"
#include "class_endian.h"
#include "interface_virtmem.h"
#include "interface_physmem.h"
#include <functional>
#include <vector>

class ARMInterpreter : public Interpreter {
	public:
	typedef std::function<bool(int coproc, int opc, uint32_t *value, int rn, int rm, int type)> CoprocReadCB;
	typedef std::function<bool(int coproc, int opc, uint32_t value, int rn, int rm, int type)> CoprocWriteCB;
	typedef std::function<bool(int value)> SoftwareInterruptCB;
	typedef std::function<bool()> UndefinedCB;
	
	enum ConditionType {
		EQ, NE, CS, CC, MI, PL, VS, VC, HI, LS, GE, LT, GT, LE, AL
	};
	
	ARMInterpreter(ARMCore *core, IPhysicalMemory *physmem, IVirtualMemory *virtmem, bool bigEndian);
	bool step();
	bool stepARM();
	bool stepThumb();
	
	uint32_t getPC() { return core->regs[ARMCore::PC]; }
	
	bool checkCondition(int type);
	
	bool handleCoprocessorRead(int coproc, int opc, uint32_t *value, int rn, int rm, int type);
	bool handleCoprocessorWrite(int coproc, int opc, uint32_t value, int rn, int rm, int type);
	bool handleSoftwareInterrupt(int value);
	bool handleUndefined();
	
	void setCoprocReadCB(CoprocReadCB callback);
	void setCoprocWriteCB(CoprocWriteCB callback);
	void setSoftwareInterruptCB(SoftwareInterruptCB callback);
	void setUndefinedCB(UndefinedCB callback);
	
	ARMCore *core;
	
	private:
	CoprocReadCB coprocReadCB;
	CoprocWriteCB coprocWriteCB;
	SoftwareInterruptCB softwareInterruptCB;
	UndefinedCB undefinedCB;
};
