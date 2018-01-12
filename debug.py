
import pyemu
import log
import time
import struct


class BreakpointHandler:
	def __init__(self, interpreter):
		self.interpreter = interpreter
		self.callbacks = {}
		self.callbacks_write = {}
		self.callbacks_read = {}
		
	def add(self, addr, callback):
		self.interpreter.add_breakpoint(addr)
		if addr not in self.callbacks:
			self.callbacks[addr] = []
		self.callbacks[addr].append(callback)
		
	def remove(self, addr, callback):
		self.interpreter.remove_breakpoint(addr)
		self.callbacks[addr].remove(callback)
		
	def watch(self, write, addr, callback):
		self.interpreter.add_watchpoint(write, addr)

		callbacks = self.callbacks_write if write else self.callbacks_read
		if addr not in callbacks:
			callbacks[addr] = []
		callbacks[addr].append(callback)
		
	def unwatch(self, write, addr, callback):
		self.interpreter.remove_watchpoint(write, addr)
		callbacks = self.callbacks_write if write else self.callbacks_read
		callbacks[addr].remove(callback)
		
	def handle(self, addr):
		for callback in self.callbacks[addr]:
			callback(addr)
			
	def handle_watch(self, addr, write):
		callbacks = self.callbacks_write if write else self.callbacks_read
		for callback in callbacks[addr]:
			callback(addr, write)

	
class MemoryReader:
	def __init__(self, physmem, virtmem):
		self.physmem = physmem
		self.virtmem = virtmem
		
	def read(self, addr, len, trans=True):
		if trans:
			addr = self.virtmem.translate(addr)
		return self.physmem.read(addr, len)
		
	def string(self, addr, trans=True):
		data = b""
		char = self.read(addr, 1, trans)
		while char != b"\0":
			data += char
			addr += 1
			char = self.read(addr, 1, trans)
		return data.decode("ascii")
		
	def u32(self, addr, trans=True):
		return struct.unpack(">I", self.read(addr, 4, trans))[0]
		
		
class MemoryWriter:
	def __init__(self, physmem, virtmem):
		self.physmem = physmem
		self.virtmem = virtmem
		
	def write(self, addr, data, trans=True):
		if trans:
			addr = self.virtmem.translate(addr, trans)
		self.physmem.write(addr, data)

			
class ArgParser:

	#States
	NEXT = 0
	SIMPLE = 1
	STRING = 2
	ESCAPE = 3

	def __init__(self):
		self.args = []
		self.argtext = ""
		self.strchar = None #" or '
		
	def parse(self, data):
		self.state = self.NEXT
		for char in data:
			self.process(char)
		if self.argtext:
			self.args.append(self.argtext)
			self.argtext = ""
			
	def process(self, char):
		if self.state == self.NEXT:
			if char != " ":
				if char in ['"', "'"]:
					self.state = self.STRING
					self.argtext = ""
					self.strchar = char
				else:
					self.state = self.SIMPLE
					self.argtext = char
					
		elif self.state == self.SIMPLE:
			if char == " ":
				self.state = self.NEXT
				self.args.append(self.argtext)
				self.argtext = ""
			else:
				self.argtext += char
				
		elif self.state == self.STRING:
			if char == self.strchar:
				self.state = self.NEXT
				self.args.append(self.argtext)
				self.argtext = ""
			elif char == "\\":
				self.state = self.ESCAPE
			else:
				self.argtext += char
				
		elif self.state == self.ESCAPE:
			if char in ['"', "'", "\\"]:
				self.argtext += char
			else:
				self.argtext += "\\" + char
			self.state = self.STRING
			
			
class Command:
	def __init__(self, min_args, max_args, func, usage):
		self.min_args = min_args
		self.max_args = max_args
		self.func = func
		self.usage = usage
		
	def call(self, args):
		if len(args) < self.min_args:
			self.print_usage()
		else:
			if len(args) > self.max_args and self.max_args != 0:
				args = args[:self.max_args - 1] + [" ".join(args[self.max_args - 1:])]
			self.func(*args)
				
	def print_usage(self):
		print("Usage: %s" %self.usage)
			

printables = "\0\n\t\"\\ abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ" \
	"1234567890!@#$%^&*()-_=+[]{}|;:'/?.,<>`~"
def is_printable(data):
	return all(chr(char) in printables for char in data)
	

class ARMDebugger:
	def __init__(self, emulator):
		self.core = emulator.core
		
		self.commands = {
			"state": Command(0, 0, self.state, "state")
		}
	
	def name(self): return "ARM"
	def pc(self): return self.core.reg(pyemu.ARMCore.PC)
	def lr(self): return self.core.reg(pyemu.ARMCore.LR)
	
	def set_option(self, option, value):
		if option == "thumb":
			core.set_thumb(bool(int(value)))
		else:
			print("Invalid option")
			
	def get_context(self):
		core = self.core
		vars = {"R%i" %i: core.reg(i) for i in range(16)}
		vars.update({
			"SP": vars["R13"],
			"LR": vars["R14"],
			"PC": vars["R15"]
		})
		return vars
	
	def eval(self, source):
		return eval(source, self.get_context())
			
	def state(self):
		core = self.core
		print("R0 = %08X R1 = %08X R2 = %08X R3 = %08X R4 = %08X" %(core.reg(0), core.reg(1), core.reg(2), core.reg(3), core.reg(4)))
		print("R5 = %08X R6 = %08X R7 = %08X R8 = %08X R9 = %08X" %(core.reg(5), core.reg(6), core.reg(7), core.reg(8), core.reg(9)))
		print("R10= %08X R11= %08X R12= %08X" %(core.reg(10), core.reg(11), core.reg(12)))
		print("SP = %08X LR = %08X PC = %08X" %(core.reg(13), core.reg(14), core.reg(15)))

		
class Module:
	def __init__(self, reader, module):
		info = reader.u32(module + 0x28)
		self.path = reader.string(reader.u32(info))
		self.name = self.path.split("\\")[-1]
		self.text = reader.u32(info + 4)
		self.textsize = reader.u32(info + 0xC)
		self.data = reader.u32(info + 0x10)
		self.datasize = reader.u32(info + 0x18)
		self.next = reader.u32(module + 0x54)
		
	def __repr__(self):
		return "<module %s>" %self.name
		

class PPCDebugger:
	def __init__(self, emulator, core_id):
		self.emulator = emulator
		self.core = emulator.core
		self.core_id = core_id
		
		self.commands = {
			"state": Command(0, 0, self.state, "state"),
			"getspr": Command(1, 1, self.getspr, "getspr <spr>"),
			"setspr": Command(2, 2, self.setspr, "setspr <spr> <value>"),
			"setpc": Command(1, 1, self.setpc, "setpc <value>"),
			"modules": Command(0, 0, self.modules, "modules"),
			"module": Command(1, 1, self.module, "module <name>"),
			"threads": Command(0, 0, self.threads, "threads"),
			"thread": Command(1, 1, self.thread, "thread <addr>"),
			"find": Command(1, 1, self.find, "find <addr>"),
			"trace": Command(0, 0, self.trace, "trace"),
			"memmap": Command(0, 0, self.memmap, "memmap")
		}
		
	def name(self): return "PPC%i" %self.core_id
	def pc(self): return self.core.pc()
	def lr(self): return self.core.spr(pyemu.PPCCore.LR)

	def get_context(self):
		core = self.core
		vars = {"r%i" %i: core.reg(i) for i in range(32)}
		vars.update({
			"sp": vars["r1"],
			"pc": core.pc(),
			"lr": core.spr(pyemu.PPCCore.LR),
			"ctr": core.spr(pyemu.PPCCore.CTR),
			"module": self.get_module_by_name
		})
		return vars
		
	def eval(self, source):
		return eval(source, self.get_context())
		
	def state(self):
		core = self.core
		print("r0 = %08X r1 = %08X r2 = %08X r3 = %08X r4 = %08X" %(core.reg(0), core.reg(1), core.reg(2), core.reg(3), core.reg(4)))
		print("r5 = %08X r6 = %08X r7 = %08X r8 = %08X r9 = %08X" %(core.reg(5), core.reg(6), core.reg(7), core.reg(8), core.reg(9)))
		print("r10= %08X r11= %08X r12= %08X r13= %08X r14= %08X" %(core.reg(10), core.reg(11), core.reg(12), core.reg(13), core.reg(14)))
		print("r15= %08X r16= %08X r17= %08X r18= %08X r19= %08X" %(core.reg(15), core.reg(16), core.reg(17), core.reg(18), core.reg(19)))
		print("r20= %08X r21= %08X r22= %08X r23= %08X r24= %08X" %(core.reg(20), core.reg(21), core.reg(22), core.reg(23), core.reg(24)))
		print("r25= %08X r26= %08X r27= %08X r28= %08X r29= %08X" %(core.reg(25), core.reg(26), core.reg(27), core.reg(28), core.reg(29)))
		print("r30= %08X r31= %08X pc = %08X lr = %08X ctr= %08X" %(core.reg(30), core.reg(31), core.pc(), core.spr(core.LR), core.spr(core.CTR)))
		print("cr = %08X" %(core.cr()))
		
	def getspr(self, spr):
		value = self.core.spr(int(spr))
		print("%08X (%i)" %(value, value))
		
	def setspr(self, spr, value):
		self.core.setspr(int(spr), self.eval(value))

	def setpc(self, value):
		self.core.setpc(self.eval(value))
		
	def get_modules(self):
		reader = self.emulator.mem_reader
	
		#Memory might not be mapped yet
		try: addr = reader.u32(0x10081018)
		except: return []
		
		modules = []
		while addr:
			module = Module(reader, addr)
			modules.append(module)
			addr = module.next
		return modules
		
	def get_module_by_name(self, name):
		for module in self.get_modules():
			if name in module.path:
				return module
		
	def get_module_by_addr(self, addr):
		for module in self.get_modules():
			if module.text <= addr < module.text + module.textsize or \
			   module.data <= addr < module.data + module.datasize:
				return module
			
	def print_stack_trace(self, sp, tabs=0):
		tabs = "\t" * tabs
		reader = self.emulator.mem_reader
		while sp:
			lr = reader.u32(sp + 4)
			if not lr: return

			module = self.get_module_by_addr(lr)
			if module:
				print("%s%08X: %s+0x%X" %(tabs, lr, module.name, lr - module.text))
			else:
				print("%s%08X" %(tabs, lr))
			sp = reader.u32(sp)
		
	def modules(self):
		for module in sorted(self.get_modules(), key=lambda m: m.text):
			print("%08X: %s" %(module.text, module.path))
			
	def module(self, name):
		for module in filter(lambda m: name in m.path, self.get_modules()):
			print("%s:" %module.path)
			print("\t.text: %08X - %08X" %(module.text, module.text + module.textsize))
			print("\t.data: %08X - %08X" %(module.data, module.data + module.datasize))
			
	def threads(self):
		reader = self.emulator.mem_reader
		thread = reader.u32(0x100567F8)
		while thread:
			name = reader.string(reader.u32(thread + 0x5C0))
			print("%08X: %s" %(thread, name))
			thread = reader.u32(thread + 0x38C)
			
	def thread(self, thread):
		thread = self.eval(thread)
		reader = self.emulator.mem_reader
		print("Name:", reader.string(reader.u32(thread + 0x5C0)))
		print("Stack trace:")
		self.print_stack_trace(reader.u32(thread + 0xC), 1)
			
	def find(self, addr):
		addr = self.eval(addr)
		module = self.get_module_by_addr(addr)
		if module:
			if module.text <= addr <= module.text + module.textsize:
				segment = "text"
				addr -= module.text
			else:
				segment = "data"
				addr -= module.data
			print("%s:%s+0x%X" %(module.name, segment, addr))
		else:
			print("Unknown")
			
	def trace(self):
		#Stack frame might be set up only partially
		lr = self.core.spr(self.core.LR)
		for addr in [self.core.pc(), lr]:
			module = self.get_module_by_addr(addr)
			if module: print("%08X: %s+0x%X" %(addr, module.name, addr - module.text))
			else: print("%08X" %addr)

		reader = self.emulator.mem_reader
		sp = reader.u32(self.core.reg(1))
		if sp and reader.u32(sp + 4) == lr:
			sp = reader.u32(sp)
		self.print_stack_trace(sp)
		
	def print_bat(self, name, batu, batl):
		if batu & 3:
			validity = {
				1: "user mode only",
				2: "supervisor only",
				3: "user/supervisor"
			}[batu & 3]
			
			access = [
				"no access", "read only", "read/write", "read only"
			][batl & 3]
			
			size = 0x20000
			sizemask = (batu >> 2) & 0x7FF
			while sizemask & 1:
				size *= 2
				sizemask >>= 1
				
			vaddr = batu & 0xFFFE0000
			paddr = batl & 0xFFFE0000
			print("\t%s: %08X-%08X => %08X-%08X (%s, %s)" %(
				name, vaddr, vaddr + size, paddr, paddr + size, access, validity
			))

		else:
			print("\t%s: disabled" %name)
		
	#Prints the virtual memory map of a PowerPC core
	def memmap(self):
		reader = self.emulator.mem_reader
		mmu = self.emulator.virtmem
		
		print("DBAT:")
		for i in range(8):
			dbatu = mmu.get_dbatu(i)
			dbatl = mmu.get_dbatl(i)
			self.print_bat("dbat%i" %i, dbatu, dbatl)
			
		print("\nIBAT:")
		for i in range(8):
			ibatu = mmu.get_ibatu(i)
			ibatl = mmu.get_ibatl(i)
			self.print_bat("ibat%i" %i, ibatu, ibatl)
		
		sdr1 = mmu.get_sdr1()
		pagetbl = sdr1 & 0xFFFF0000
		pagemask = sdr1 & 0x1FF
		
		print("\nPage table (%08X):" %pagetbl)

		pagemap = []
		current = None
		for addr in range(0, 0x100000000, 0x20000):
			sr = mmu.get_sr(addr >> 28)
			if sr >> 31: #Direct-store
				if current: pagemap.append(current)
				current = None

			else:
				ks = (sr >> 30) & 1
				kp = (sr >> 29) & 1
				nx = (sr >> 28) & 1
				vsid = sr & 0xFFFFFF
				
				pageidx = (addr >> 17) & 0x1FFFF
				hash = (vsid & 0x7FFFF) ^ pageidx
				hash &= (pagemask << 10) | 0x3FF
				api = pageidx >> 5
				
				pte = pagetbl | (hash << 6)
				for i in range(8):
					ptehi = reader.u32(pte + i * 8, False)
					ptelo = reader.u32(pte + i * 8 + 4, False)
					
					if not ptehi >> 31: continue
					if (ptehi >> 6) & 1: continue
					if (ptehi >> 7) & 0xFFFFFF != vsid: continue
					if (ptehi & 0x3F) != api: continue
					
					pp = ptelo & 3
					phys = ptelo & 0xFFFFF000
					
					if current and current[2:] == [ks, kp, nx, pp, phys - current[1]]:
						current[1] += 0x20000
					else:
						if current: pagemap.append(current)
						current = [addr, 0x20000, ks, kp, nx, pp, phys]
					break

				else:
					if current: pagemap.append(current)
					current = None
					
		if current: pagemap.append(current)
		
		if pagemap:
			for start, size, ks, kp, nx, pp, phys in pagemap:
				access_prot = [
					["read/write", "read/write", "read/write", "read only"],
					["no access", "read only", "read/write", "read only"]
				]
				user_access = "user: %s" %access_prot[kp][pp]
				kernel_access = "supervisor: %s" %access_prot[ks][pp]
				no_execute = "no execute" if nx else "executable"
				print("\t%08X-%08X => %08X-%08X (%s, %s, %s)" %(
					start, start + size, phys, phys + size, user_access, kernel_access, no_execute
				))
		else:
			print("no pages mapped")

		
class DebugShell:
	def __init__(self, scheduler):
		self.scheduler = scheduler
		self.current_override = None
		self.input_interrupt = False
		
		self.commands = {
			"help": Command(0, 1, self.help, "help (<command>)"),
			"select": Command(1, 1, self.select, "select <index>"),
			"break": Command(2, 2, self.breakp, "break add/del <address>"),
			"watch": Command(3, 3, self.watch, "watch add/del read/write <address>"),
			"read": Command(3, 3, self.read, "read phys/virt <address> <length>"),
			"translate": Command(1, 2, self.translate, "translate <address> (type)"),
			"getreg": Command(1, 1, self.getreg, "getreg <reg>"),
			"setreg": Command(2, 2, self.setreg, "setreg <reg> <value>"),
			"step": Command(0, 0, self.step, "step"),
			"eval": Command(1, 1, self.evalcmd, "eval <expr>")
		}
		
	def current(self):
		if self.current_override:
			return self.current_override
		return self.scheduler.current
		
	def handle_breakpoint(self, addr):
		self.current_override = None
		name = self.current().debugger.name()
		print("%s: Breakpoint hit at %08X\n" %(name, addr))
		self.show(True)
			
	def handle_watchpoint(self, addr, write):
		self.current_override = None
		debugger = self.current().debugger
		type = "write" if write else "read"
		print("%s: Watchpoint %08X (%s) hit at %08X\n" %(debugger.name(), addr, type, debugger.pc()))
		self.show(True)
		
	def show(self, is_break):
		#Catching KeyboardInterrupts is quite messy
		while True:
			debugger = self.current().debugger

			try:
				try: inp = input("%s:%08X> " %(debugger.name(),  debugger.pc()))
				except EOFError: #Apparently this is a Windows problem
					time.sleep(2) #Wait for keyboard interrupt
					self.input_interrupt = True
					raise
			except KeyboardInterrupt:
				self.input_interrupt = True
				raise #Exit

			try:
				parser = ArgParser()
				parser.parse(inp)
				if parser.args:
					cmd, args = parser.args[0], parser.args[1:]
					if cmd == "run":
						if not is_break:
							self.scheduler.run()
						return
					else:
						self.execute_command(cmd, args)

			except KeyboardInterrupt as e:
				#Exit if Ctrl+C is pressed during input()
				if self.input_interrupt: raise

				#But show the debugger if Ctrl+C is pressed while the interpreter is running
				print("Keyboard interrupt")
				
			except Exception as e:
				print("%s: %s" %(e.__class__.__name__, e))
				
	def get_command(self, cmd):
		for plugin in [self, self.current().debugger]:
			if cmd in plugin.commands:
				return plugin.commands[cmd]
		print("Unknown command")

	def execute_command(self, cmd, args):
		command = self.get_command(cmd)
		if command:
			command.call(args)

	def eval(self, source):
		return self.current().debugger.eval(source)

	def help(self, cmd=None):
		debugger = self.current().debugger

		if cmd:
			command = self.get_command(cmd)
			if command:
				command.print_usage()
				
		else:
			print()
			for plugin in [self, debugger]:
				print("%s:" %plugin.__class__.__name__)
				for cmd in sorted(plugin.commands.keys()):
					print("\t%s" %cmd)
				print()

	def select(self, index):
		self.current_override = self.scheduler.emulators[int(index)]

	def breakp(self, op, address):
		func = {
			"add": self.current().breakpoints.add,
			"del": self.current().breakpoints.remove
		}[op]
		func(self.eval(address), self.handle_breakpoint)

	def watch(self, op, type, address):
		func = {
			"add": self.current().breakpoints.watch,
			"del": self.current().breakpoints.unwatch
		}[op]
		
		write = {
			"read": False,
			"write": True
		}[type]

		func(write, self.eval(address), self.handle_watchpoint)

	def read(self, type, address, length):
		address = self.eval(address)
		if type == "virt":
			address = self.current().virtmem.translate(address)

		data = self.current().physmem.read(address, self.eval(length))
		print(data.hex())
		if is_printable(data):
			print(data.decode("ascii"))

	def translate(self, address, type=pyemu.IVirtualMemory.DATA_READ):
		addr = self.current().virtmem.translate(self.eval(address), type)
		print("0x%08X" %addr)
		
	def getreg(self, reg):
		value = self.current().core.reg(int(reg))
		print("0x%08X (%i)" %(value, value))
		
	def setreg(self, reg, value):
		self.current().core.setreg(int(reg), self.eval(value))

	def step(self):
		self.current().interpreter.step()
		
	def evalcmd(self, source):
		result = self.eval(source)
		if type(result) == int:
			print("%08X (%i)" %(result, result))
		else:
			print(result)
