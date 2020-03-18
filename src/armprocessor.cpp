
#include "emulator.h"
#include "armprocessor.h"
#include "arminstruction.h"
#include "armthumbinstr.h"
#include "strings.h"

#include "common/exceptions.h"
#include "common/logger.h"


ARMProcessor::ARMProcessor(Emulator *emulator) :
	Processor(emulator, 0),
	mmu(&emulator->physmem, &core),
	jit(&emulator->physmem, this),
	thumbJit(&emulator->physmem, this)
{
	printer.init("ARM");
	
	console.init("logs/iosu.txt");
	syslog.init("logs/syslog.txt");
}

void ARMProcessor::reset() {
	core.reset();
	mmu.reset();
	jit.reset();
	thumbJit.reset();
	
	timer = 100;
	
	#if STATS
	thumbInstrs = 0;
	armInstrs = 0;
	#endif
}

void ARMProcessor::step() {
	if (core.isThumb()) {
		stepThumb();
	}
	else {
		stepARM();
	}
	
	updateTimer();
	checkDebugPoints();
	
	#if BREAKPOINTS
	checkBreakpoints(core.regs[ARMCore::PC]);
	#endif
}

void ARMProcessor::stepThumb() {
	uint32_t addr = core.regs[ARMCore::PC];
	
	bool supervisor = core.getMode() != ARMCore::User;
	if (mmu.translate(&addr, MemoryAccess::Instruction, supervisor)) {
		thumbJit.execute(addr);
		#if STATS
		thumbInstrs++;
		#endif
	}
	else {
		core.triggerException(ARMCore::PrefetchAbort);
	}
}

void ARMProcessor::stepARM() {
	uint32_t addr = core.regs[ARMCore::PC];
	
	bool supervisor = core.getMode() != ARMCore::User;
	if (mmu.translate(&addr, MemoryAccess::Instruction, supervisor)) {
		jit.execute(addr);
		#if STATS
		armInstrs++;
		#endif
	}
	else {
		core.triggerException(ARMCore::PrefetchAbort);
	}
}

void ARMProcessor::checkDebugPoints() {
	if (core.regs[ARMCore::PC] == 0xD400AEC) {
		std::string message = physmem->read<std::string>(core.regs[ARMCore::R0]);
		
		int args = 0;
		for (char c : message) {
			if (c == '%') {
				args++;
			}
		}
		
		std::string text;
		if (args == 0) text = message;
		else if (args == 1) text = StringUtils::format(message, core.regs[ARMCore::R1]);
		else {
			Logger::warning("Boot1 debug message has unsupported number of arguments");
			return;
		}
		
		printer.write(text);
		console.write(text);
	}
	else if (core.regs[ARMCore::PC] == 0x812DD20) {
		uint16_t instr;
		read(core.regs[ARMCore::LR] - 2, &instr);
		
		if (instr == 0xDFAB) {
			int op = core.regs[ARMCore::R0];
			
			if (op == 4) { //SYS_WRITE0
				uint32_t addr = core.regs[ARMCore::R1];
				mmu.translate(&addr, MemoryAccess::DataRead, true);
				
				std::string message = physmem->read<std::string>(addr);
				printer.write(message);
				console.write(message);
			}
			
			else {
				Logger::warning("Unknown semihosting operation: %i", op);
			}
		}
	}
	#if SYSLOG
	else if (core.regs[ARMCore::PC] == 0x5055294) {
		uint32_t addr = core.regs[ARMCore::R1];
		mmu.translate(&addr, MemoryAccess::DataRead, true);
		
		std::string message = physmem->read<std::string>(addr);
		syslog.write(message);
	}
	#endif
}

void ARMProcessor::updateTimer() {
	if (--timer == 0) {
		hardware->update();
		checkInterrupts();
		timer = 100;
	}
}

void ARMProcessor::checkInterrupts() {
	if (hardware->check_interrupts_arm()) {
		core.triggerException(ARMCore::InterruptRequest);
	}
}

bool ARMProcessor::coprocessorRead(int coproc, int opc, uint32_t *value, int rn, int rm, int type) {
	if (coproc == 15) { // System control coprocessor
		if (rn == 1) { // Control register
			if (type == 0) { // Control register
				*value = core.control;
				return true;
			}
		}
		else if (rn == 5) { // Fault status
			if (type == 0) {
				*value = core.dfsr;
				return true;
			}
			else if (type == 1) {
				*value = core.ifsr;
				return true;
			}
		}
		else if (rn == 6) { // Fault address register
			*value = core.far;
			return true;
		}
		else if (rn == 7) { // Cache management functions
			if (rm == 10 && type == 3) { // Test and clean
				*value = ARMCore::Z;
				return true;
			}
			else if (rm == 14 && type == 3) { // Test, clean and invalidate
				*value = ARMCore::Z;
				return true;
			}
		}
	}
	Logger::warning("Unknown coprocessor read: %i %i %i %i %i", coproc, opc, rn, rm, type);
	return false;
}

bool ARMProcessor::coprocessorWrite(int coproc, int opc, uint32_t value, int rn, int rm, int type) {
	if (coproc == 15) { // System control coprocessor
		if (rn == 1) { // Control register
			if (type == 0) { // Control register
				core.control = value;
				return true;
			}
		}
		else if (rn == 2) { // Translation table base
			if (type == 0) {
				core.ttbr = value;
				return true;
			}
		}
		else if (rn == 3) { // Domain access control
			core.domain = value;
			return true;
		}
		else if (rn == 5) { // Fault status
			if (type == 0) {
				core.dfsr = value;
				return true;
			}
			else if (type == 1) {
				core.ifsr = value;
				return true;
			}
		}
		else if (rn == 6) { // Fault address register
			if (type == 0) {
				core.far = value;
				return true;
			}
		}
		else if (rn == 7) { // Cache management functions
			if (rm == 0 && type == 4) { // Wait for interrupt
				while (!hardware->check_interrupts_arm()) {
					hardware->update();
				}
				core.triggerException(ARMCore::InterruptRequest);
				return true;
			}
			else if (rm == 5 && type == 0) { // Invalidate entire instruction cache
				jit.invalidate();
				thumbJit.invalidate();
				return true;
			}
			else if (rm == 6 && type == 0) return true; // Invalidate entire data cache
			else if (rm == 6 && type == 1) return true; // Invalidate data cache line
			else if (rm == 10 && type == 1) return true; // Clean data cache line
			else if (rm == 10 && type == 4) return true; // Data synchronization barrier
		}
		else if (rn == 8) { // TLB functions
			if (rm == 7 && type == 0) {
				mmu.cache.invalidate(); // Invalidate entire instruction and data TLBs
				return true;
			}
		}
	}
	
	Logger::warning("Unknown coprocessor write: %i %i %i %i %i (0x%08X)", coproc, opc, rn, rm, type, value);
	return false;
}
