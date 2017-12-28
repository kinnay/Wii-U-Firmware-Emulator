
from Crypto.Cipher import AES

import pyemu
import struct
import sys

import armemu
import ppcemu
import hardware
import debug


class Scheduler:
	def __init__(self):
		self.emulators = []
		self.running = []
		self.steps = {}
		self.current = None

	def add(self, emulator, steps):
		if not self.current:
			self.current = emulator
		self.emulators.append(emulator)
		self.steps[emulator] = steps
		
	def resume(self, emulator):
		if emulator not in self.running:
			self.running.append(emulator)
			
	def pause(self, emulator):
		if emulator in self.running:
			self.running.remove(emulator)

	def run(self):
		index = 0
		while True:
			self.current = self.running[index]
			self.current.check_interrupts()
			self.current.interpreter.run(self.steps[self.current])

			index += 1
			if index >= len(self.running):
				index = 0
				
	def pc(self):
		return self.current.debugger.pc()
				
				
class Emulator:
	def __init__(self, filename):
		self.scheduler = Scheduler()
	
		self.init_physmem()
		self.init_hardware()
		self.init_cpu()
		self.init_debugger()
		self.init_elffile(filename)
		
		#Fix these hacks
		self.armemu.breakpoints.add(0x5015E70, lambda addr: self.armemu.core.setreg(0, 0))
		self.armemu.breakpoints.add(0x503409C, lambda addr: self.reset_ppc())
		
	def init_physmem(self):
		self.physmem = pyemu.PhysicalMemory()
		self.physmem.add_range(0x10000000, 0x18000000) #IOSU processes and ram disk
		self.physmem.add_range(0x08000000, 0x002E0000) #MEM0
		self.physmem.add_range(0xFFF00000, 0x000FF000) #Kernel stuff
		self.physmem.add_range(0x30000000, 0x02800000) #Root and loader
		self.physmem.add_range(0x00000000, 0x02000000) #MEM1

	def init_hardware(self):
		self.hw = hardware.HardwareController(self.scheduler, self.physmem)
		self.physmem.add_special(0xD000000, 0x1A0000, self.hw.read, self.hw.write)
		self.physmem.add_special(0xD800000, 0x1A0000, self.hw.read, self.hw.write)
		self.physmem.add_special(0xC000000, 0x400000, self.hw.read, self.hw.write)
		
	def init_cpu(self):
		reservation = pyemu.PPCLockMgr()

		self.armemu = armemu.ARMEmulator(self, self.physmem, self.hw)
		self.ppcemu = [ppcemu.PPCEmulator(self.physmem, self.hw, reservation, core) for core in range(3)]

		self.scheduler.add(self.armemu, 1000)
		for emu in self.ppcemu:
			self.scheduler.add(emu, 1000)
		self.scheduler.resume(self.armemu)
		
	def init_debugger(self):
		self.shell = debug.DebugShell(self.scheduler)
		self.armemu.interpreter.on_watchpoint(True, self.shell.handle_watchpoint)
		self.armemu.interpreter.on_watchpoint(False, self.shell.handle_watchpoint)
		for emu in self.ppcemu:
			emu.interpreter.on_watchpoint(True, self.shell.handle_watchpoint)
			emu.interpreter.on_watchpoint(False, self.shell.handle_watchpoint)
		
	def init_elffile(self, filename):
		with open(filename, "rb") as f:
			data = f.read()
		elf = pyemu.ELFFile(data)
		
		for program in elf.programs:
			if program.type == pyemu.ELFProgram.PT_LOAD:
				if program.filesize:
					self.physmem.write(program.physaddr, data[program.fileoffs : program.fileoffs + program.filesize])

		self.armemu.core.setreg(pyemu.ARMCore.PC, elf.entry_point)
			
	def reset_ppc(self):
		size = struct.unpack(">I", self.physmem.read(0x080000AC, 4))[0]
		data = self.physmem.read(0x08000100, size)

		with open("espresso_key.txt") as f:
			key = bytes.fromhex(f.read())
		cipher = AES.new(key, AES.MODE_CBC, bytes(16))
		self.physmem.write(0x08000100, cipher.decrypt(data))
		
		for emu in self.ppcemu:
			emu.core.setpc(0xFFF00100)
			self.scheduler.resume(emu)

	def print_traceback(self):
		import traceback
		traceback.print_exc()

		print("An error occurred:")
		print("\tARM: PC=%08X LR=%08X%s" %(self.armemu.core.reg(15), self.armemu.core.reg(14), " (current)" if self.scheduler.current == self.armemu else ""))
		for i in range(3):
			emu = self.ppcemu[i]
			current = " (current)" if emu == self.scheduler.current else ""
			print("\tPPC%i: PC=%08X LR=%08X%s" %(i, emu.core.pc(), emu.core.spr(pyemu.PPCCore.LR), current))
		
	def run(self):
		try:
			if "break" in sys.argv: self.shell.show(False)
			else: self.scheduler.run()
		except:
			self.print_traceback()
		
		self.armemu.cleanup()
		for e in self.ppcemu: e.cleanup()
		self.hw.nand.close()
		self.hw.sdio1.close()

		
if __name__ == "__main__":
	emulator = Emulator("C:/Users/Yannik/Downloads/openssl/fwdec.elf")
	emulator.run()
