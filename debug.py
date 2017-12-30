
import pyemu
import log


class BreakpointHandler:
	def __init__(self, interpreter):
		self.interpreter = interpreter
		self.callbacks = {}
		
	def add(self, addr, callback):
		self.interpreter.add_breakpoint(addr)
		if addr not in self.callbacks:
			self.callbacks[addr] = []
		self.callbacks[addr].append(callback)
		
	def remove(self, addr, callback):
		self.interpreter.remove_breakpoint(addr)
		self.callbacks[addr].remove(callback)
		
	def handle(self, addr):
		for callback in self.callbacks[addr]:
			callback(addr)


printables = "\0\n\t\"\\ abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ" \
	"1234567890!@#$%^&*()-_=+[]{}|;:'/?.,<>`~"
def is_printable(data):
	return all(chr(char) in printables for char in data)
	
	
class ARMDebugger:
	def __init__(self, emulator):
		self.core = emulator.core
	
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
			
	def print_state(self):
		core = self.core
		print("R0 = %08X R1 = %08X R2 = %08X R3 = %08X R4 = %08X" %(core.reg(0), core.reg(1), core.reg(2), core.reg(3), core.reg(4)))
		print("R5 = %08X R6 = %08X R7 = %08X R8 = %08X R9 = %08X" %(core.reg(5), core.reg(6), core.reg(7), core.reg(8), core.reg(9)))
		print("R10= %08X R11= %08X R12= %08X" %(core.reg(10), core.reg(11), core.reg(12)))
		print("SP = %08X LR = %08X PC = %08X" %(core.reg(13), core.reg(14), core.reg(15)))
	
	
class PPCDebugger:
	def __init__(self, emulator, core_id):
		self.core = emulator.core
		self.core_id = core_id
		
	def name(self): return "PPC%i" %self.core_id
	def pc(self): return self.core.pc()
	def lr(self): return self.core.spr(pyemu.PPCCore.LR)
	
	def set_option(self, option, value):
		print("Invalid option")
		
	def get_context(self):
		core = self.core
		vars = {"r%i" %i: core.reg(i) for i in range(32)}
		vars.update({
			"sp": vars["r1"],
			"pc": core.pc(),
			"lr": core.spr(pyemu.PPCCore.LR),
			"ctr": core.spr(pyemu.PPCCore.CTR)
		})
		return vars
		
	def print_state(self):
		core = self.core
		print("r0 = %08X r1 = %08X r2 = %08X r3 = %08X r4 = %08X" %(core.reg(0), core.reg(1), core.reg(2), core.reg(3), core.reg(4)))
		print("r5 = %08X r6 = %08X r7 = %08X r8 = %08X r9 = %08X" %(core.reg(5), core.reg(6), core.reg(7), core.reg(8), core.reg(9)))
		print("r10= %08X r11= %08X r12= %08X r13= %08X r14= %08X" %(core.reg(10), core.reg(11), core.reg(12), core.reg(13), core.reg(14)))
		print("r15= %08X r16= %08X r17= %08X r18= %08X r19= %08X" %(core.reg(15), core.reg(16), core.reg(17), core.reg(18), core.reg(19)))
		print("r20= %08X r21= %08X r22= %08X r23= %08X r24= %08X" %(core.reg(20), core.reg(21), core.reg(22), core.reg(23), core.reg(24)))
		print("r25= %08X r26= %08X r27= %08X r28= %08X r29= %08X" %(core.reg(25), core.reg(26), core.reg(27), core.reg(28), core.reg(29)))
		print("r30= %08X r31= %08X pc = %08X lr = %08X ctr= %08X" %(core.reg(30), core.reg(31), core.pc(), core.spr(core.LR), core.spr(core.CTR)))
		print("cr = %08X" %(core.cr()))
		
		
class DebugShell:
	def __init__(self, scheduler):
		self.scheduler = scheduler
		self.current_override = None

	def get_current(self):
		if self.current_override:
			return self.current_override
		return self.scheduler.current

	def handle_breakpoint(self, addr):
		self.current_override = None
		name = self.get_current().debugger.name()
		print("%s: Breakpoint hit at %08X\n" %(name, addr))
		self.show(True)
			
	def handle_watchpoint(self, addr):
		self.current_override = None
		debugger = self.get_current().debugger
		print("%s: Watchpoint %08X hit at %08X\n" %(debugger.name(), addr, debugger.pc()))
		self.show(True)
		
	def show(self, is_break):
		while True:
			debugger = self.get_current().debugger
			command = input("%s:%08X> " %(debugger.name(), debugger.pc()))
			try:
				result = self.execute_command(*command.split(" "))
				if result:
					self.current_override = None
					if not is_break:
						self.scheduler.run()
					return
			except Exception:
				import traceback
				traceback.print_exc()
			
	def execute_command(self, cmd, *args):
		current = self.get_current()
		physmem = current.physmem
		virtmem = current.virtmem
		breakpoints = current.breakpoints
		interpreter = current.interpreter
		debugger = current.debugger
		core = current.core
	
		if cmd == "select":
			if len(args) != 1:
				print("Usage: core <index>")
			else:
				index = int(args[0])
				self.current_override = self.scheduler.emulators[index]
	
		elif cmd == "break":
			if len(args) < 2:
				print("Usage: break (add/del) <address>")
			else:
				addr = self.eval(args[1:])
				if args[0] == "add":
					breakpoints.add(addr, self.handle_breakpoint)
				elif args[0] == "del":
					breakpoints.remove(addr, self.handle_breakpoint)
				else:
					print("Usage: break (add/del) <address>")
					
		elif cmd == "watch":
			addr = self.eval(args[1:])
			write = {
				"write": True,
				"read": False
			}[args[0]]
			interpreter.add_watchpoint(write, addr)
					
		elif cmd == "phys":
			if len(args) != 3:
				print("Usage: phys read <address> <length>")
			else:
				if args[0] == "read":
					addr = self.eval([args[1]])
					length = self.eval([args[2]])
					data = physmem.read(addr, length)
					print(data.hex())
					if is_printable(data):
						print(data.decode("ascii"))
				else:
					print("Usage: phys read <address> <length>")
					
		elif cmd == "virt":
			if len(args) != 3:
				print("Usage: virt read <address> <length>")
			else:
				if args[0] == "read":
					addr = virtmem.translate(self.eval([args[1]]))
					length = self.eval([args[2]])
					data = physmem.read(addr, length)
					print(data.hex())
					if is_printable(data):
						print(data.decode("ascii"))
				else:
					print("Usage: virt read <address> <length>")
					
		elif cmd == "translate":
			if not args:
				print("Usage: translate <addr> (type)")
			else:				
				type = pyemu.IVirtualMemory.DATA_READ
				if len(args) == 2:
					type = self.eval(args[1])
				addr = virtmem.translate(self.eval(args[:1]), type)
				print("0x%08X" %addr)
					
		elif cmd == "getreg":
			if len(args) != 1:
				print("Usage: getreg <reg>")
			else:
				value = core.reg(int(args[0]))
				print("0x%08X (%i)" %(value, value))
			
		elif cmd == "setreg":
			if len(args) < 2:
				print("Usage: setreg <reg> <value>")
			else:
				core.setreg(int(args[0]), self.eval(args[1:]))
					
		elif cmd == "set":
			if len(args) != 2:
				print("Usage: set <option> <value>")
			else:
				debugger.set_option(args[1], args[2])
				
		elif cmd == "run": return True
		elif cmd == "step": interpreter.step()
		elif cmd == "state": debugger.print_state()
			
		elif cmd == "eval":
			result = self.eval(args)
			if type(result) == int:
				print("0x%08X (%i)" %(result, result))
			elif result is not None:
				print(result)
		
		else:
			print("Unknown command")
			
	def eval(self, strings):
		string = " ".join(strings)
		debugger = self.get_current().debugger
		return eval(" ".join(strings), debugger.get_context())
