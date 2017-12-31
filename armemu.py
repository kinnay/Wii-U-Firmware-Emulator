
import pyemu
import struct
import sys

import debug
import log

class CoprocHandler:
	def __init__(self, core, mmu):
		self.core = core
		self.mmu = mmu

		self.control_reg = 0
		self.domain_access_reg = 0
		self.data_fault_status = 0
		self.instr_fault_status = 0
		self.fault_address = 0

	def write(self, coproc, opc, value, rn, rm, type):
		if coproc == 15:
			fields = [rn, rm, type]
			if fields == [1, 0, 0]: #Write control register
				self.control_reg = value
				self.mmu.set_enabled(value & 1)
			elif fields == [2, 0, 0]: #Write translation table base register 0
				self.mmu.set_translation_table_base(value)
			elif fields == [3, 0, 0]: #Write domain access control register
				self.domain_access_reg = value
			elif fields == [5, 0, 0]: #Write data fault status register
				self.data_fault_status = value
			elif fields == [5, 0, 1]: #Write instruction fault status register
				self.instr_fault_status = value
			elif fields == [6, 0, 0]: #Write fault address register
				self.fault_address = value
			elif fields == [7, 0, 4]: pass #Wait for interrupt
			elif fields == [7, 5, 0]: pass #Invalidate instruction cache line register
			elif fields == [7, 6, 0]: pass #Invalidate data cache line register
			elif fields == [7, 6, 1]: pass #Invalidate data cache line register
			elif fields == [7, 10, 1]: pass #Data synchronization barrier register
			elif fields == [7, 10, 4]: pass #Clean and invalidate entire data cache register
			elif fields == [8, 7, 0]: pass #Invalidate unified TLB register
			else:
				print("COPROC WRITE p%i %i %08X c%i c%i %i at %08X" %(coproc, opc, value, rn, rm, type, self.core.reg(15)))
		else:
			raise RuntimeError("Write to coprocessor %i" %coproc)
		
	def read(self, coproc, opc, rn, rm, type):
		if coproc == 15:
			fields = [rn, rm, type]
			if fields == [1, 0, 0]: #Read control register
				return self.control_reg
			elif fields == [3, 0, 0]: #Read domain access control register
				return self.domain_access_reg
			elif fields == [5, 0, 0]: #Read data fault status register
				return self.data_fault_status
			elif fields == [5, 0, 1]: #Read instruction fault status register
				return self.instr_fault_status
			elif fields == [6, 0, 0]: #Read fault address register
				return self.fault_address
			elif fields == [7, 10, 3]: #Test and clean dcache
				return pyemu.ARMCore.Z
			elif fields == [7, 14, 3]: #Test, clean and invalidate dcache
				return pyemu.ARMCore.Z
			else:
				print("COPROC READ p%i %i c%i c%i %i at %08X" %(coproc, opc, rn, rm, type, self.core.reg(15)))
				return 0
		else:
			raise RuntimeError("Read from coprocessor %i" %coproc)
			
			
class MemoryReader:
	def __init__(self, physmem, virtmem):
		self.physmem = physmem
		self.virtmem = virtmem
		
	def read(self, addr, len):
		addr = self.virtmem.translate(addr)
		return self.physmem.read(addr, len)
		
	def string(self, addr):
		data = b""
		char = self.read(addr, 1)
		while char != b"\0":
			data += char
			addr += 1
			char = self.read(addr, 1)
		return data.decode("ascii")
		
	def u32(self, addr):
		return struct.unpack(">I", self.read(addr, 4))[0]
		
		
class MemoryWriter:
	def __init__(self, physmem, virtmem):
		self.physmem = physmem
		self.virtmem = virtmem
		
	def write(self, addr, data):
		addr = self.virtmem.translate(addr)
		self.physmem.write(addr, data)
			
			
class SVCHandler:
	def __init__(self, core, reader, writer):
		self.core = core
		self.reader = reader
		self.writer = writer
		
		self.logger = log.ConsoleLogger("ARM")
		self.next_handle = 1000

	def handle(self, id):
		if id != 0xAB:
			raise RuntimeError("Invalid software interrupt: 0x%X" %id)

		type = self.core.reg(0)
		if type == 1: #Open(name, mode, namelen)
			ptr = self.core.reg(1)
			name = self.reader.string(self.u32(ptr))
			mode = self.reader.u32(ptr + 4)
			print("OPEN(%s, %i) -> %i" %(name, mode, self.next_handle))
			self.core.setreg(0, self.next_handle)
			self.next_handle += 1

		elif type == 2: #Close
			print("CLOSE(%i)" %self.reader.u32(self.core.reg(1)))

		elif type == 4: #Write
			self.logger.write(self.reader.string(self.core.reg(1)))

		elif type == 6: #Read
			ptr = self.core.reg(1)
			handle = self.reader.u32(ptr)
			buffer = self.reader.u32(ptr + 4)
			length = self.reader.u32(ptr + 8)
			result = input("READ(%i, %i): " %(handle, length))
			result = result[:length - 1] + "\0"
			self.writer.write(buffer, result.encode("ascii"))
			self.core.setreg(0, len(result))

		else:
			raise RuntimeError("Invalid svc type: %i" %type)
			
			
class UNDHandler:

	IOS_CREATE_MESSAGE_QUEUE = 0xC
	IOS_DESTROY_MESSAGE_QUEUE = 0xD
	IOS_SEND_MESSAGE = 0xE
	IOS_JAM_MESSAGE = 0xF
	IOS_RECEIVE_MESSAGE = 0x10
	IOS_OPEN = 0x33
	IOS_CLOSE = 0x34
	IOS_READ = 0x35
	IOS_WRITE = 0x36
	IOS_SEEK = 0x37
	IOS_IOCTL = 0x38
	IOS_IOCTLV = 0x39
	IOS_OPEN_ASYNC = 0x3A
	IOS_CLOSE_ASYNC = 0x3B
	IOS_READ_ASYNC = 0x3C
	IOS_WRITE_ASYNC = 0x3D
	IOS_SEEK_ASYNC = 0x3E
	IOS_IOCTL_ASYNC = 0x3F
	IOS_IOCTLV_ASYNC = 0x40
	IOS_RESUME = 0x43
	IOS_RESUME_ASYNC = 0x46
	IOS_RESOURCE_REPLY = 0x49

	def __init__(self, breakpoints, core, reader, writer):
		self.breakpoints = breakpoints
		self.core = core
		self.reader = reader
		self.writer = writer

		self.requests = []
		self.async_reqs = []
		self.added_pcs = []
		self.ipc_names = {}
		
		self.ipc_logger = log.FileLogger("ipc.txt")
		self.message_logger = log.FileLogger("messages.txt")
		self.file_logger = log.FileLogger("files.txt")

	def handle(self):
		if "logsys" in sys.argv:
			self.log_syscall() #Slows down code a lot
		self.core.trigger_exception(self.core.UNDEFINED_INSTRUCTION)
		
	def handle_breakpoint(self, pc):
		for req in self.requests:
			if pc == req[0] and self.get_thread() == req[1]:
				self.handle_result(pc, *req[2:])
				self.requests.remove(req)
				return
				
	def handle_result(self, pc, syscall, *args):
		module = self.get_module_name(pc)
		result = self.core.reg(0)
		lr = self.core.reg(14)
		pc = self.core.reg(15)
		
		if syscall == self.IOS_CREATE_MESSAGE_QUEUE:
			self.message_logger.print("[%s:%08X] CREATE(%i) -> %08X" %(module, lr, args[0], result))
			
		elif syscall == self.IOS_RECEIVE_MESSAGE:
			queue = args[0]
			message = self.reader.u32(args[1])
			flags = args[2]
			self.message_logger.print("[%s:%08X] RECEIVE(%08X, %i) -> (%08X, %08X)" %(module, lr, queue, flags, result, message))
			
			for req in self.async_reqs:
				if queue == req[2] and message == req[3]:
					result = self.reader.u32(message + 4)
					self.handle_async_result(pc, req[1], req[4], result, *req[5:])
					self.async_reqs.remove(req)
					break
			
		
		elif syscall == self.IOS_OPEN:
			self.ipc_logger.print("[%s:%08X] OPEN(%s, %i) -> %08X" %(module, lr, *args, result))
			self.ipc_names[result] = args[0]
			
		elif syscall == self.IOS_IOCTL:
			self.ipc_logger.print("[%s:%08X] IOCTL[%s](%08X, %i) -> %08X" %(module, lr, *args[:3], result))

			dev, fd, ioctl, indata, outdata = args
			self.handle_ioctl(dev, ioctl, indata, outdata, result)

		elif syscall == self.IOS_IOCTLV:
			self.ipc_logger.print("[%s:%08X] IOCTLV[%s](%08X, %i) -> %08X" %(module, lr, *args[:3], result))
			
			dev, fd, ioctlv, vectors = args
			self.handle_ioctlv(dev, ioctlv, vectors, result)
					
		elif syscall == self.IOS_RESUME:
			self.ipc_logger.print("[%s:%08X] RESUME[%s](%08X) -> %08X" %(module, lr, *args, result))
			
	def handle_async_result(self, pc, lr, syscall, result, *args):
		module = self.get_module_name(pc)
		if syscall == self.IOS_IOCTL_ASYNC:
			fd, ioctl, indata, outdata = args
			name = self.ipc_names[fd]
			
			self.ipc_logger.print("[%s:%08X] IOCTL_ASYNC[%s](%08X, %i) -> %08X" %(module, lr, name, fd, ioctl, result))
			self.handle_ioctl(name, ioctl, indata, outdata, result)
		
		elif syscall == self.IOS_IOCTLV_ASYNC:
			fd, ioctlv, vectors = args
			name = self.ipc_names[fd]
		
			self.ipc_logger.print("[%s:%08X] IOCTLV_ASYNC[%s](%08X, %i) -> %08X" %(module, lr, name, fd, ioctlv, result))
			self.handle_ioctlv(name, ioctlv, vectors, result)
		
		elif syscall == self.IOS_RESUME_ASYNC:
			fd = args[0]
			self.ipc_logger.print("[%s:%08X] RESUME_ASYNC[%s](%08X) -> %08X" %(module, lr, self.ipc_names[fd], fd, result))
			
	def handle_ioctl(self, name, ioctl, indata, outdata, result):
		if name == "/dev/fsa":
			if ioctl == 3: #FSAGetVolumeInfo
				path = indata[4 : 4 + 0x280].decode("ascii").strip("\0")
				self.ipc_logger.print("\tFSAGetVolumeInfo(%s)" %path)
			elif ioctl == 4: #FSAInit
				self.ipc_logger.print("\tFSAInit()")
			elif ioctl == 5: #FSAChangeDir
				path = indata[4 : 4 + 0x280].decode("ascii").strip("\0")
				self.ipc_logger.print("\tFSAChangeDir(%s)" %path)
			elif ioctl == 7: #FSAMakeDir
				path = indata[4 : 4 + 0x280].decode("ascii").strip("\0")
				arg = struct.unpack_from(">I", indata, 0x284)[0]
				self.ipc_logger.print("\tFSAMakeDir(%s, %i)" %(path, arg))
			elif ioctl == 8: #FSARemove
				path = indata[4 : 4 + 0x280].decode("ascii").strip("\0")
				self.ipc_logger.print("\tFSARemove(%s)" %path)
			elif ioctl == 10: #FSAOpenDir
				path = indata[4 : 4 + 0x280].decode("ascii").strip("\0")
				self.ipc_logger.print("\tFSAOpenDir(%s)" %path)
			elif ioctl == 14: #FSAOpenFile
				fn = indata[4 : 4 + 0x280].decode("ascii").strip("\0")
				mode = indata[0x284 : 0x284 + 0x10].decode("ascii").strip("\0")
				self.ipc_logger.print("\tFSAOpenFile(%s, %s)" %(fn, mode))
				self.file_logger.print("FSAOpenFile(%s, %s)" %(fn, mode))
			elif ioctl == 20: #FSAGetStatFile
				handle = indata[4 : 8].hex().upper()
				self.ipc_logger.print("\tFSAGetStatFile(%s)" %handle)
			elif ioctl == 24: #FSAGetInfoByQuery
				fn = indata[4 : 4 + 0x280].decode("ascii").strip("\0")
				type = {
					0: "FSAGetFreeSpaceSize",
					1: "FSAGetDirSize",
					2: "FSAGetEntryNum",
					4: "FSAGetDeviceInfo",
					5: "FSAGetStat",
					7: "FSAGetJournalFreeSpaceSize"
				}[struct.unpack_from(">I", indata, 0x284)[0]]
				self.ipc_logger.print("\tFSAGetInfoByQuery(%s, %s)" %(fn, type))
			
	def handle_ioctlv(self, name, ioctlv, vectors, result):
		if name == "/dev/crypto":
			if ioctlv == 12: #IOSC_GenerateHash
				type = struct.unpack_from(">I", vectors[0], 12)[0]
				datalen = len(vectors[2])
				hash = vectors[3]
				self.ipc_logger.print("\tIOSC_GenerateHash(0x%X, %i) -> %s" %(datalen, type, hash.hex()))
			elif ioctlv == 14: #IOSC_Decrypt
				key = struct.unpack_from(">I", vectors[0], 8)[0]
				iv = vectors[1]
				datalen = len(vectors[2])
				self.ipc_logger.print("\tIOSC_Decrypt(%i, 0x%X, %s)" %(key, datalen, iv.hex()))
			if ioctlv == 16: #IOSC_GenerateBlockMAC
				key, type = struct.unpack_from(">II", vectors[0], 8)
				datalen = len(vectors[3])
				customlen = len(vectors[2])
				hash = vectors[4]
				self.ipc_logger.print("\tIOSC_GenerateBlockMAC(%i, 0x%X, 0x%X, %i) -> %s" %(key, datalen, customlen, type, hash.hex()))
				
		if name == "/dev/fsa":
			if ioctlv == 1: #FSAMount
				data = vectors[0]
				path1 = data[4 : 4 + 0x280].decode("ascii").strip("\0")
				path2 = data[0x284 : 0x284 + 0x280].decode("ascii").strip("\0")
				self.ipc_logger.print("\tFSAMount(%s, %s)" %(path1, path2))
			elif ioctlv == 15: #FSAReadFile
				data = vectors[0]
				length, count = struct.unpack_from(">II", data, 8)
				self.ipc_logger.print("\tFSAReadFile(0x%X * %i) -> 0x%X" %(count, length, result))
			elif ioctlv == 103:
				data = vectors[0]
				string1 = data[4 : 4 + 0x280].decode("ascii").strip("\0")
				string2 = data[0x284 : 0x284 + 0x280].decode("ascii").strip("\0")
				self.ipc_logger.print("\tFSA_0x67(%s, %s, ...)" %(string1, string2))
			
	def add_request(self, pc, *args):
		if pc not in self.added_pcs:
			self.breakpoints.add(pc, self.handle_breakpoint)
			self.added_pcs.append(pc)
		self.requests.append((pc, self.get_thread()) + args)
		
	def add_async(self, *args):
		self.async_reqs.append(args)
		
	def log_syscall(self):
		core = self.core
		pc = core.reg(15)
		lr = core.reg(14)

		instr = self.reader.u32(pc - 4)
		if instr & ~0xFF00 == 0xE7F000F0:
			syscall = (instr >> 8) & 0xFF
			module = self.get_module_name(pc)

			if syscall == self.IOS_CREATE_MESSAGE_QUEUE:
				self.add_request(pc, syscall, core.reg(1))
			elif syscall == self.IOS_DESTROY_MESSAGE_QUEUE:
				self.message_logger.print("[%s:%08X] DESTROY(%08X)" %(module, lr, core.reg(0)))
			elif syscall == self.IOS_SEND_MESSAGE:
				args = self.get_args(3)
				self.message_logger.print("[%s:%08X] SEND(%08X, %08X, %i)" %(module, lr, *args))
			elif syscall == self.IOS_JAM_MESSAGE:
				args = self.get_args(3)
				self.message_logger.print("[%s:%08X] JAM(%08X, %08X, %i)" %(module, lr, *args))
			elif syscall == self.IOS_RECEIVE_MESSAGE:
				self.add_request(pc, syscall, *self.get_args(3))

			elif syscall == self.IOS_OPEN:
				name = self.reader.string(core.reg(0))
				mode = core.reg(1)
				self.add_request(pc, syscall, name, mode)
			elif syscall == self.IOS_CLOSE:
				fd = core.reg(0)
				self.ipc_logger.print("[%s:%08X] CLOSE[%s](%08X)" %(module, lr, self.ipc_names[fd], fd))
				self.ipc_names.pop(fd)
			elif syscall == self.IOS_IOCTL:
				args = self.get_args(6)
				fd, ioctl = args[0], args[1]
				indata = self.reader.read(args[2], args[3])
				outdata = self.reader.read(args[4], args[5])
				self.add_request(pc, syscall, self.ipc_names[fd], fd, ioctl, indata, outdata)
			elif syscall == self.IOS_IOCTLV:
				args = self.get_args(5)
				
				fd, ioctlv = args[0], args[1]
				
				offs = args[4]
				vectors = []
				for i in range(args[2] + args[3]):
					ptr, size = self.reader.u32(offs), self.reader.u32(offs + 4)
					vectors.append(self.reader.read(ptr, size))
					offs += 12

				self.add_request(pc, syscall, self.ipc_names[fd], fd, ioctlv, vectors)

			elif syscall == self.IOS_IOCTL_ASYNC:
				args = self.get_args(8)
				fd, ioctl = args[0], args[1]
				indata = self.reader.read(args[2], args[3])
				outdata = self.reader.read(args[4], args[5])
				self.add_async(pc, lr, *args[6:], syscall, fd, ioctl, indata, outdata)
			elif syscall == self.IOS_IOCTLV_ASYNC:
				args = self.get_args(7)

				fd, ioctlv = args[0], args[1]
				
				offs = args[4]
				vectors = []
				for i in range(args[2] + args[3]):
					ptr, size = self.reader.u32(offs), self.reader.u32(offs + 4)
					vectors.append(self.reader.read(ptr, size))
					offs += 12

				self.add_async(pc, lr, *args[5:], syscall, fd, ioctlv, vectors)

			elif syscall == self.IOS_RESUME:
				fd = core.reg(0)
				self.add_request(pc, syscall, self.ipc_names[fd], fd)
			elif syscall == self.IOS_RESUME_ASYNC:
				args = self.get_args(5)
				self.add_async(pc, lr, *args[3:], syscall, args[0])
				
	def get_args(self, num):
		args = []
		for i in range(num):
			if i <= 3:
				args.append(self.core.reg(i))
			else:
				args.append(self.reader.u32(self.core.reg(13) + (i - 4) * 4))
		return args
				
	def get_thread(self):
		return self.reader.u32(0x8173BA0)
			
	def get_module_name(self, addr):
		if 0x04000000 <= addr < 0x04020000: return "CRYPTO"
		if 0x05000000 <= addr < 0x05060000: return "MCP"
		if 0x08120000 <= addr < 0x08140000: return "KERNEL"
		if 0x10100000 <= addr < 0x10140000: return "USB"
		if 0x10700000 <= addr < 0x10800000: return "FS"
		if 0x11F00000 <= addr < 0x11FC0000: return "PAD"
		if 0x12300000 <= addr < 0x12440000: return "NET"
		if 0xE0000000 <= addr < 0xE0100000: return "ACP"
		if 0xE1000000 <= addr < 0xE10C0000: return "NSEC"
		if 0xE2000000 <= addr < 0xE2280000: return "NIM_BOSS"
		if 0xE3000000 <= addr < 0xE3180000: return "FPD"
		if 0xE4000000 <= addr < 0xE4040000: return "TEST"
		if 0xE5000000 <= addr < 0xE5040000: return "AUXIL"
		if 0xE6000000 <= addr < 0xE6040000: return "BSP"
		raise ValueError("get_module_name(0x%08X)" %addr)
		

class ExceptionHandler:
	def __init__(self, core):
		self.core = core
		
	def data_abort(self, addr, write):
		if "abort" in sys.argv:
			self.data_fault_status = (write << 11) | 5
			self.fault_address = addr
			self.core.trigger_exception(self.core.DATA_ABORT)
		else:
			type = ["read from", "write to"][write]
			raise RuntimeError("Data abort: %s %08X at %08X" %(type, addr, self.core.reg(15)))

			
class ARMEmulator:
	def __init__(self, emulator, physmem, hw):
		self.emulator = emulator
		self.core = pyemu.ARMCore()
		self.physmem = physmem
		self.virtmem = pyemu.ARMMMU(physmem, True)
		self.virtmem.set_cache_enabled(True)
		self.interpreter = pyemu.ARMInterpreter(self.core, physmem, self.virtmem, True)
		self.interpreter.set_alarm(5000, hw.latte.update_timer)
		self.interrupts = hw.latte.irq_arm
		
		self.mem_reader = MemoryReader(physmem, self.virtmem)
		self.mem_writer = MemoryWriter(physmem, self.virtmem)
		self.breakpoints = debug.BreakpointHandler(self.interpreter)
		self.coproc_handler = CoprocHandler(self.core, self.virtmem)
		self.svc_handler = SVCHandler(self.core, self.mem_reader, self.mem_writer)
		self.und_handler = UNDHandler(self.breakpoints, self.core, self.mem_reader, self.mem_writer)
		self.exc_handler = ExceptionHandler(self.core)
		
		self.interpreter.on_software_interrupt(self.svc_handler.handle)
		self.interpreter.on_undefined_instruction(self.und_handler.handle)
		self.interpreter.on_data_error(self.exc_handler.data_abort)
		self.interpreter.on_coproc_read(self.coproc_handler.read)
		self.interpreter.on_coproc_write(self.coproc_handler.write)
		self.interpreter.on_breakpoint(self.breakpoints.handle)
		self.interpreter.on_watchpoint(False, self.breakpoints.handle_watch)
		self.interpreter.on_watchpoint(True, self.breakpoints.handle_watch)
		
		self.debugger = debug.ARMDebugger(self)

		self.breakpoints.add(0x5055324, self.handle_syslog)
		self.logger = log.FileLogger("log.txt")
		
	def check_interrupts(self):
		if self.interrupts.check_interrupts():
			self.core.trigger_exception(self.core.IRQ)
		
	def handle_syslog(self, addr):
		addr, length = self.core.reg(1), self.core.reg(2)
		data = self.mem_reader.read(addr, length).decode("ascii")
		self.logger.write(data)
		
	def cleanup(self):
		self.logger.close()
		self.und_handler.ipc_logger.close()
		self.und_handler.message_logger.close()
		self.und_handler.file_logger.close()
		self.svc_handler.logger.close()
