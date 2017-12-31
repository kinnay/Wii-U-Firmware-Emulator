
import pyemu
import debug
import log


class SPRHandler:

	pvr = 0x70010201

	def __init__(self, core, mmu, breakpoints, core_id):
		self.core = core
		self.mmu = mmu
		self.breakpoints = breakpoints
		self.upir = core_id

		self.dec = 0xFFFFFFFF
		self.tb = 0
		self.sprg0 = 0
		self.sprg1 = 0
		self.sprg2 = 0
		self.sprg3 = 0
		self.hid2 = 0
		self.wpar = 0
		self.hid5 = 0
		self.scr = 0
		self.car = 0
		self.bcr = 0
		self.mmcr0 = 0
		self.pmc1 = 0
		self.pmc2 = 0
		self.mmcr1 = 0
		self.pmc3 = 0
		self.pmc4 = 0
		self.dcate = 0
		self.dcatr = 0
		self.dmatl0 = 0
		self.dmatu0 = 0
		self.dmatr0 = 0
		self.dmatl1 = 0
		self.dmatu1 = 0
		self.dmatr1 = 0
		self.hid0 = 0
		self.hid1 = 0
		self.iabr = 0
		self.hid4 = 0
		self.dabr = 0
		self.l2cr = 0
		self.thrm3 = 0

	def read(self, spr):
		if spr == 268: return self.tb & 0xFFFFFFFF
		elif spr == 269: return self.tb >> 32
		elif spr == 272: return self.sprg0
		elif spr == 273: return self.sprg1
		elif spr == 274: return self.sprg2
		elif spr == 275: return self.sprg3
		elif spr == 287: return self.pvr
		elif 528 <= spr <= 535:
			if spr % 2: return self.mmu.get_ibatl((spr - 528) // 2)
			else: return self.mmu.get_ibatu((spr - 528) // 2)
		elif 536 <= spr <= 543:
			if spr % 2: return self.mmu.get_dbatl((spr - 536) // 2)
			else: return self.mmu.get_dbatu((spr - 536) // 2)
		elif 560 <= spr <= 567:
			if spr % 2: return self.mmu.get_ibatl((spr - 560) // 2 + 4)
			else: return self.mmu.get_ibatu((spr - 560) // 2 + 4)
		elif 568 <= spr <= 575:
			if spr % 2: return self.mmu.get_dbatl((spr - 568) // 2 + 4)
			else: return self.mmu.get_dbatu((spr - 568) // 2 + 4)
		elif spr == 905: return self.wpar
		elif spr == 920: return self.hid2
		elif spr == 921: return self.wpar
		elif spr == 936: return self.mmcr0
		elif spr == 937: return self.pmc1
		elif spr == 938: return self.pmc2
		elif spr == 940: return self.mmcr1
		elif spr == 941: return self.pmc3
		elif spr == 942: return self.pmc4
		elif spr == 944: return self.hid5
		elif spr == 947: return self.scr
		elif spr == 948: return self.car
		elif spr == 1007: return self.upir
		elif spr == 1008: return self.hid0
		elif spr == 1009: return self.hid1
		elif spr == 1011: return self.hid4
		elif spr == 1017: return self.l2cr
		elif spr == 1022: return self.thrm3
		print("SPR READ %i at %08X" %(spr, self.core.pc()))
		return 0
		
	def write(self, spr, value):
		if spr == 22: self.dec = value
		elif spr == 25: self.mmu.set_sdr1(value)
		elif spr == 272: self.sprg0 = value
		elif spr == 273: self.sprg1 = value
		elif spr == 274: self.sprg2 = value
		elif spr == 275: self.sprg3 = value
		elif spr == 284: self.tb = (self.tb & 0xFFFFFFFF00000000) | value
		elif spr == 285: self.tb = (self.tb & 0xFFFFFFFF) | (value << 32)
		elif 528 <= spr <= 535:
			if spr % 2: self.mmu.set_ibatl((spr - 528) // 2, value)
			else: self.mmu.set_ibatu((spr - 528) // 2, value)
		elif 536 <= spr <= 543:
			if spr % 2: self.mmu.set_dbatl((spr - 536) // 2, value)
			else: self.mmu.set_dbatu((spr - 536) // 2, value)
		elif 560 <= spr <= 567:
			if spr % 2: self.mmu.set_ibatl((spr - 560) // 2 + 4, value)
			else: self.mmu.set_ibatu((spr - 560) // 2 + 4, value)
		elif 568 <= spr <= 575:
			if spr % 2: self.mmu.set_dbatl((spr - 568) // 2 + 4, value)
			else: self.mmu.set_dbatu((spr - 568) // 2 + 4, value)
		elif spr == 920: self.hid2 = value
		elif spr == 921: self.wpar = value
		elif spr == 944: self.hid5 = value
		elif spr == 947: self.scr = value
		elif spr == 948: self.car = value
		elif spr == 949: self.bcr = value
		elif spr == 952: self.mmcr0 = value
		elif spr == 953: self.pmc1 = value
		elif spr == 954: self.pmc2 = value
		elif spr == 956: self.mmcr1 = value
		elif spr == 957: self.pmc3 = value
		elif spr == 958: self.pmc4 = value
		elif spr == 976: self.dcate = value
		elif spr == 977: self.dcatr = value
		elif spr == 984: self.dmatl0 = value
		elif spr == 985: self.dmatu0 = value
		elif spr == 986: self.dmatr0 = value
		elif spr == 987: self.dmatl1 = value
		elif spr == 988: self.dmatu1 = value
		elif spr == 989: self.dmatr1 = value
		elif spr == 1008: self.hid0 = value
		elif spr == 1010: #IABR
			#Remove previous breakpoint
			if self.iabr & 2: self.breakpoints.remove(self.iabr & ~3, self.handle_iabr)
			
			self.iabr = value
			if value & 2: self.breakpoints.add(value & ~3, self.handle_iabr)
		elif spr == 1011: self.hid4 = value
		elif spr == 1013: #DABR
			#Remove previous watchpoints
			if self.dabr & 1: self.breakpoints.unwatch(False, self.dabr & ~7, self.handle_dabr)
			if self.dabr & 2: self.breakpoints.unwatch(True, self.dabr & ~7, self.handle_dabr)
			
			self.dabr = value
			if value & 1: self.breakpoints.watch(False, value & ~7, self.handle_dabr)
			if value & 2: self.breakpoints.watch(True, value & ~7, self.handle_dabr)
		elif spr == 1017: self.l2cr = value
		elif spr == 1022: self.thrm3 = value
		else:
			print("SPR WRITE %i %08X at %08X" %(spr, value, self.core.pc()))
			
	def handle_dabr(self, addr, write):
		if (self.core.msr() & 0x10) >> 4 == (self.dabr & 4) >> 2:
			raise NotImplementedError("Data address breakpoint hit")
			
	def handle_iabr(self, addr):
		if (self.core.msr() & 0x20) >> 5 == self.iabr & 1:
			raise NotImplementedError("Instruction address breakpoint hit")
			
			
class MSRHandler:
	def __init__(self, mmu):
		self.mmu = mmu

	def write(self, value):
		self.mmu.set_data_translation(value & 0x10)
		self.mmu.set_instruction_translation(value & 0x20)
		self.mmu.set_supervisor(not value & 0x4000)
		
		
class ExceptionHandler:
	def __init__(self, core):
		self.core = core

	def handle_dsi(self, addr, write):
		print("DSI exception: %08X at %08X" %(addr, self.core.pc()))
		self.core.setspr(self.core.DAR, addr)
		self.core.setspr(self.core.DSISR, 0x40000000 | (write * 0x02000000))
		self.core.trigger_exception(self.core.DSI)
	
	def handle_isi(self, addr):
		print("ISI exception: %08X" %addr)
		self.core.trigger_exception(self.core.ISI)


class PPCEmulator:
	def __init__(self, physmem, hw, reservation, core_id):
		self.core = pyemu.PPCCore(reservation)
		self.translator = pyemu.VirtualMemory()
		self.translator.add_range(0x00000000, 0x00000000, 0xFFE00000)
		self.translator.add_range(0xFFE00000, 0x08000000, 0x00200000)
		self.physmem = pyemu.PhysicalMirror(physmem, self.translator)
		self.virtmem = pyemu.PPCMMU(self.physmem, True)
		self.virtmem.set_cache_enabled(True)
		self.virtmem.set_rpn_size(15) #This is so weird
		self.interpreter = pyemu.PPCInterpreter(self.core, self.physmem, self.virtmem, True)
		self.interrupts = hw.pi[core_id]

		self.logger = log.PrintLogger("PPC")
		self.breakpoints = debug.BreakpointHandler(self.interpreter)
		self.breakpoints.add(0xFFF1AB34, self.handle_log)
		self.spr_handler = SPRHandler(self.core, self.virtmem, self.breakpoints, core_id)
		self.msr_handler = MSRHandler(self.virtmem)
		self.exc_handler = ExceptionHandler(self.core)

		self.interpreter.on_fetch_error(self.exc_handler.handle_isi)
		self.interpreter.on_data_error(self.exc_handler.handle_dsi)
		self.interpreter.on_breakpoint(self.breakpoints.handle)
		self.interpreter.on_watchpoint(False, self.breakpoints.handle_watch)
		self.interpreter.on_watchpoint(True, self.breakpoints.handle_watch)
		self.interpreter.set_alarm(5000, self.update_timer)
		
		self.core.on_spr_read(self.spr_handler.read)
		self.core.on_spr_write(self.spr_handler.write)
		self.core.on_msr_write(self.msr_handler.write)
		self.core.on_sr_read(self.virtmem.get_sr)
		self.core.on_sr_write(self.virtmem.set_sr)
		
		self.debugger = debug.PPCDebugger(self, core_id)
		
	def check_interrupts(self):
		if self.interrupts.check_interrupts():
			self.core.trigger_exception(self.core.EXTERNAL_INTERRUPT)
		
	def handle_log(self, addr):
		data = self.physmem.read(self.core.reg(6), self.core.reg(7)).decode("ascii")
		self.logger.write(data)
		
	def update_timer(self):
		self.spr_handler.tb = (self.spr_handler.tb + 400) & 0xFFFFFFFFFFFFFFFF
		
		self.spr_handler.dec -= 400
		if self.spr_handler.dec < 0:
			self.spr_handler.dec += 0x100000000
			self.core.trigger_exception(self.core.DECREMENTER)
			
	def cleanup(self):
		self.logger.close()
