
#include "class_ppcinterp.h"
#include "class_ppccore.h"
#include "class_ppcinstr.h"

PPCInterpreter::PPCInterpreter(PPCCore *core, PhysicalMemory *physmem, IVirtualMemory *virtmem)
	: Interpreter(physmem, virtmem, true), core(core)
{
	core->setMsrWriteCB([this](uint32_t value) -> bool { return this->handleMsrWrite(value); });
}

bool PPCInterpreter::handleMsrWrite(uint32_t value) {
	virtmem->setDataTranslation(value & 0x10);
	virtmem->setInstructionTranslation(value & 0x20);
	virtmem->setSupervisorMode(!(value & 0x4000));
	return true;
}

bool PPCInterpreter::step() {
	PPCInstruction instr;
	if (!readCode<uint32_t>(core->pc, &instr.value)) return false;

	core->pc += 4;
	
	return instr.execute(this);
}
