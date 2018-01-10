
#include "class_ppcinterp.h"
#include "class_ppccore.h"
#include "class_ppcinstr.h"
#include "class_endian.h"

#include <algorithm>

PPCInterpreter::PPCInterpreter(PPCCore *core, IPhysicalMemory *physmem, IVirtualMemory *virtmem, bool bigEndian)
	: Interpreter(physmem, virtmem, bigEndian), core(core) {}

bool PPCInterpreter::step() {
	PPCInstruction instr;
	if (!readCode<uint32_t>(core->pc, &instr.value)) return false;

	core->pc += 4;
	
	PPCInstrCallback func = instr.decode();
	if (func) {
		return func(this, instr);
	}
	return false;
}
