
#include "ppcprocessor.h"
#include "ppcinstruction.h"
#include "emulator.h"

#include "common/logger.h"


PPCProcessor::PPCProcessor(Emulator *emulator, PPCReservation *reservation, int index) :
	Processor(emulator, index + 1),
	mmu(&emulator->physmem, &core),
	jit(&emulator->physmem, this)
{
	this->reservation = reservation;
	
	wg = &emulator->hardware.pi.wg[index];
	
	printer.init("PPC");
	
	coslog.init("logs/coslog.txt");
}

void PPCProcessor::reset() {
	jit.reset();
	core.reset();
	core.sprs[PPCCore::PIR] = index - 1;
	core.sprs[PPCCore::PVR] = 0x70010201;
	
	#if STATS
	instrsExecuted = 0;
	#endif
	
	#if METRICS
	metrics.reset();
	#endif
	
	timer = 0;
}

void PPCProcessor::step() {
	uint32_t addr = core.pc;
	
	bool supervisor = !(core.msr & 0x4000);
	if (mmu.translate(&addr, MemoryAccess::Instruction, supervisor)) {
		#if STATS
		instrsExecuted++;
		#endif
		
		core.pc += 4;
		jit.execute(addr);
	}
	else {
		core.triggerException(PPCCore::ISI);
	}
	
	updateTimer();
	checkDebugPoints();
	
	#if BREAKPOINTS
	checkBreakpoints(core.pc);
	#endif
}

void PPCProcessor::checkDebugPoints() {
	if (core.pc == 0xFFF1AB34) {
		uint32_t addr = core.regs[6];
		uint32_t length = core.regs[7];
		mmu.translate(&addr, MemoryAccess::DataRead, true);
		
		Buffer data = physmem->read(addr, length);
		std::string string = data.tostring();
		if (length > 0 && string[length - 1] != '\n') {
			string += '\n';
		}
		
		printer.write(string);
		coslog.write(string);
	}
}

void PPCProcessor::updateTimer() {
	core.sprs[PPCCore::TBL]++;
	if (core.sprs[PPCCore::TBL] == 0) {
		core.sprs[PPCCore::TBU]++;
	}
	
	core.sprs[PPCCore::DEC]--;
	if (core.sprs[PPCCore::DEC] == -1) {
		core.triggerException(PPCCore::Decrementer);
	}
	
	timer++;
	if (timer == 100) {
		checkInterrupts();
		timer = 0;
	}
}

void PPCProcessor::checkInterrupts() {
	if (hardware->check_interrupts_ppc(index - 1)) {
		core.triggerException(PPCCore::ExternalInterrupt);
	}
}
