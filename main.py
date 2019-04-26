
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
		self.scheduler = pyemu.Scheduler()
		self.scheduler.add_alarm(1, self.check_interrupts)
		self.emulators = []
		
	def add(self, emulator, steps):
		self.emulators.append(emulator)
		self.scheduler.add(emulator.interpreter, steps)
		
	def resume(self, emulator):
		self.scheduler.resume(self.emulators.index(emulator))
		
	def run(self): self.scheduler.run()
	def add_alarm(self, interval, callback):
		self.scheduler.add_alarm(interval, callback)
		
	def current(self): return self.emulators[self.scheduler.index]
	def pc(self): return self.current().debugger.pc()
		
	def check_interrupts(self):
		for emulator in self.emulators:
			emulator.check_interrupts()
				
				
class Emulator:
	def __init__(self, filename):
		self.scheduler = Scheduler()
		self.shell = debug.DebugShell(self.scheduler)

		self.init_physmem()
		self.init_hardware()
		self.init_cpu()
		self.init_elffile(filename)

		self.armemu.breakpoints.add(0x5015E70, lambda addr: self.armemu.core.setreg(0, 0))
		self.armemu.breakpoints.add(0x503409C, lambda addr: self.reset_ppc())
		
	def init_physmem(self):
		self.physmem = pyemu.PhysicalMemory()
		self.physmem.add_range(0x10000000, 0x28000000) #IOSU processes and ram disk
		self.physmem.add_range(0x08000000, 0x082E0000) #MEM0
		self.physmem.add_range(0xFFF00000, 0xFFFFF000) #Kernel stuff
		self.physmem.add_range(0x30000000, 0x32800000) #Root and loader
		self.physmem.add_range(0x00000000, 0x02000000) #MEM1

	def init_hardware(self):
		self.hw = hardware.HardwareController(self.scheduler, self.physmem)
		self.physmem.add_special(0xD000000, 0xD1A0000, self.hw.read, self.hw.write)
		self.physmem.add_special(0xD800000, 0xD9A0000, self.hw.read, self.hw.write)
		self.physmem.add_special(0xC000000, 0xC400000, self.hw.read, self.hw.write)
		self.physmem.add_special(0xD0000000, 0xD0000004, self.hw.read, self.hw.write) #Used by tcl.rpl
		
	def init_cpu(self):
		reservation = pyemu.PPCLockMgr()

		self.armemu = armemu.ARMEmulator(self, self.physmem, self.hw)
		self.ppcemu = [ppcemu.PPCEmulator(self, self.physmem, self.hw, reservation, core) for core in range(3)]

		self.scheduler.add(self.armemu, 1000)
		self.scheduler.add(self.ppcemu[0], 500)
		self.scheduler.add(self.ppcemu[1], 2000)
		self.scheduler.add(self.ppcemu[2], 500)
		self.scheduler.resume(self.armemu)
		
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
