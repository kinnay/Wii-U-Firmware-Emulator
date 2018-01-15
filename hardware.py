
from Crypto.Cipher import AES
import struct
import sys

import pyemu

if "noprint" in sys.argv:
	def print(*args): pass

	
AHMN_D8B0010 = 0xD0B0010
AHMN_MEM0_CONFIG = 0xD0B0800
AHMN_MEM1_CONFIG = 0xD0B0804
AHMN_MEM2_CONFIG = 0xD0B0808
AHMN_RDBI_MASK = 0xD0B080C
AHMN_ERROR_MASK = 0xD0B0820
AHMN_ERROR = 0xD0B0824
AHMN_D8B0840 = 0xD0B0840
AHMN_D8B0844 = 0xD0B0844
AHMN_TRANSFER_STATE = 0xD0B0850
AHMN_WORKAROUND = 0xD0B0854
AHMN_MEM0_START = 0xD0B0900
AHMN_MEM1_START = 0xD0B0A00
AHMN_MEM2_START = 0xD0B0C00
AHMN_MEM0_END = 0xD0B0980
AHMN_MEM1_END = 0xD0B0C00
AHMN_MEM2_END = 0xD0B1000

AHMN_START = 0xD0B0000
AHMN_END = 0xD0B1000

class AHMNController:
	def __init__(self, scheduler):	
		self.mem0_config = 0
		self.mem1_config = 0
		self.mem2_config = 0
		self.workaround = 0
		self.mem0 = [0] * ((AHMN_MEM0_END - AHMN_MEM0_START) // 4)
		self.mem1 = [0] * ((AHMN_MEM1_END - AHMN_MEM1_START) // 4)
		self.mem2 = [0] * ((AHMN_MEM2_END - AHMN_MEM2_START) // 4)
		
		self.scheduler = scheduler

	def read(self, addr):
		if addr == AHMN_MEM0_CONFIG: return self.mem0_config
		elif addr == AHMN_MEM1_CONFIG: return self.mem1_config
		elif addr == AHMN_MEM2_CONFIG: return self.mem2_config
		elif addr == AHMN_TRANSFER_STATE: return 0
		elif addr == AHMN_WORKAROUND: return self.workaround
		elif AHMN_MEM0_START <= addr < AHMN_MEM0_END: return self.mem0[(addr - AHMN_MEM0_START) // 4]
		elif AHMN_MEM1_START <= addr < AHMN_MEM1_END: return self.mem1[(addr - AHMN_MEM1_START) // 4]
		elif AHMN_MEM2_START <= addr < AHMN_MEM2_END: return self.mem2[(addr - AHMN_MEM2_START) // 4]
		
		print("AHMN READ 0x%X at %08X" %(addr, self.scheduler.pc()))
		value = 0
	
	def write(self, addr, value):
		if addr == AHMN_D8B0010: pass
		elif addr == AHMN_MEM0_CONFIG: self.mem0_config = value
		elif addr == AHMN_MEM1_CONFIG: self.mem1_config = value
		elif addr == AHMN_MEM2_CONFIG: self.mem2_config = value
		elif addr == AHMN_RDBI_MASK: pass
		elif addr == AHMN_ERROR_MASK: pass
		elif addr == AHMN_ERROR: pass
		elif addr == AHMN_D8B0840: pass
		elif addr == AHMN_D8B0844: pass
		elif addr == AHMN_WORKAROUND: self.workaround = value
		elif AHMN_MEM0_START <= addr < AHMN_MEM0_END: self.mem0[(addr - AHMN_MEM0_START) // 4] = value ^ 0x80000000
		elif AHMN_MEM1_START <= addr < AHMN_MEM1_END: self.mem1[(addr - AHMN_MEM1_START) // 4] = value ^ 0x80000000
		elif AHMN_MEM2_START <= addr < AHMN_MEM2_END: self.mem2[(addr - AHMN_MEM2_START) // 4] = value ^ 0x80000000
		else:
			print("AHMN WRITE 0x%X %08X at %08X" %(addr, value, self.scheduler.pc()))
			

MEM_D8B4200 = 0xD0B4200
MEM_FLUSH_MASK = 0xD0B4228
MEM_EDRAM_REFRESH_CTRL = 0xD0B42CC
MEM_EDRAM_REFRESH_VAL = 0xD0B42CE
MEM_BLOCK_MEM0_CFG = 0xD0B4400
MEM_BLOCK_MEM1_CFG = 0xD0B4402
MEM_BLOCK_MEM2_CFG = 0xD0B4404
MEM_UNK_START = 0xD0B440E
MEM_UNK_END = 0xD0B44C6
MEM_D8B44E6 = 0xD0B44E6
MEM_D8B44E8 = 0xD0B44E8
MEM_D8B44EA = 0xD0B44EA

MEM_START = 0xD0B4000
MEM_END = 0xD0B4800
			
class MEMController:
	def __init__(self, scheduler):
		self.d8b4200 = 0
		self.mem0_config = 0
		self.mem1_config = 0
		self.mem2_config = 0
		
		self.scheduler = scheduler
		
	def read(self, addr):
		if addr == MEM_D8B4200: return self.d8b4200
		elif addr == MEM_FLUSH_MASK: return 0
		elif addr == MEM_BLOCK_MEM0_CFG: return self.mem0_config
		elif addr == MEM_BLOCK_MEM1_CFG: return self.mem1_config
		elif addr == MEM_BLOCK_MEM2_CFG: return self.mem2_config

		print("MEM READ 0x%X at %08X" %(addr, self.scheduler.pc()))
		return 0

	def write(self, addr, value):
		if addr == MEM_D8B4200: self.d8b4200 = value
		elif addr == MEM_FLUSH_MASK: pass
		elif addr == MEM_EDRAM_REFRESH_CTRL: pass
		elif addr == MEM_EDRAM_REFRESH_VAL: pass
		elif addr == MEM_BLOCK_MEM0_CFG: self.mem0_config = value
		elif addr == MEM_BLOCK_MEM1_CFG: self.mem1_config = value
		elif addr == MEM_BLOCK_MEM2_CFG: self.mem2_config = value
		elif MEM_UNK_START <= addr < MEM_UNK_END: pass
		elif addr == MEM_D8B44E6: pass
		elif addr == MEM_D8B44E8: pass
		elif addr == MEM_D8B44EA: pass
		else:
			print("MEM WRITE 0x%X %08X at %08X" %(addr, value, self.scheduler.pc()))
			
			
DI2SATA_CVR = 0xD006004
		
DI2SATA_START = 0xD006000
DI2SATA_END = 0xD00602C
		
class DI2SATAController:
	def __init__(self, scheduler):
		self.scheduler = scheduler

	def read(self, addr):
		if addr == DI2SATA_CVR: return 1 #No disc?
	
		print("DI2SATA READ 0x%X at %08X" %(addr, self.scheduler.pc()))
		return 0
		
	def write(self, addr, value):
		print("DI2SATA WRITE 0x%X %08X at %08X" %(addr, value, self.scheduler.pc()))
			
			
class RTCController:
	def __init__(self, scheduler):
		self.sram = [0] * 0x10
		
		self.data = 0
		self.temp_data = 0
		self.write_state = 0
		self.write_offset = 0
		
		self.on_timer = 0
		self.off_timer = 0
		self.control_reg0 = 0
		self.control_reg1 = 0
		
		self.scheduler = scheduler
		
	def update_data(self):
		self.data = self.temp_data
		
	def handle(self, value):
		if self.write_state:
			self.write_state -= 1
			self.handle_write(value)
	
		elif value & 0x80000000:
			self.write_state = 0x10 if value == 0xA0000100 else 1
			self.write_offset = value & ~0x80000000
		else:
			self.temp_data = self.handle_read(value)
			
	def handle_read(self, offset):
		if offset == 0x21000000: return self.on_timer
		elif offset == 0x21000100: return self.off_timer
		elif offset == 0x21000C00: return self.control_reg0
		elif offset == 0x21000D00: return self.control_reg1
		else:
			print("RTC READ %08X at %08X" %(offset, self.scheduler.pc()))
			return 0
			
	def handle_write(self, value):
		if self.write_offset == 0x20000100:
			self.sram[-self.write_state] = value
		elif self.write_offset == 0x21000000:
			self.on_timer = value & 0x3FFFFFFF
		elif self.write_offset == 0x21000100:
			self.off_timer = value & 0x3FFFFFFF
		elif self.write_offset == 0x21000D00:
			if value & 0x10000:
				raise NotImplementedError("RTC power off")
			if value & 0x100:
				raise NotImplementedError("RTC sleep mode")
			self.control_reg1 = value
		else:
			print("RTC WRITE %08X %08X at %08X" %(self.write_offset, value, self.scheduler.pc()))
		
		
EXI0_CSR = 0xD006800
EXI0_CR = 0xD00680C
EXI0_DATA = 0xD006810

EXI_START = 0xD006800
EXI_END = 0xD00683C

class EXIController:
	def __init__(self, scheduler, armirq):
		self.rtc = RTCController(scheduler)
	
		self.scheduler = scheduler
		self.armirq = armirq

		self.csr0 = 0
		self.data0 = 0

	def read(self, addr):
		if addr == EXI0_CSR: return self.csr0
		elif addr == EXI0_DATA: return self.rtc.data

		print("EXI READ 0x%X at %08X" %(addr, self.scheduler.pc()))
		return 0
		
	def write(self, addr, value):
		if addr == EXI0_CSR: self.csr0 = value
		elif addr == EXI0_CR:
			if value == 0x31: #Update in data
				self.rtc.update_data()
				self.armirq.trigger_irq_all(20)
			elif value == 0x35: #Send data
				self.rtc.handle(self.data0)
				self.armirq.trigger_irq_all(20)
				self.csr0 |= 4
			else:
				print("EXI0_CR WRITE %08X at %08X" %(value, self.scheduler.pc()))

		elif addr == EXI0_DATA:
			self.data0 = value

		else:
			print("EXI WRITE 0x%X %08X at %08X" %(addr, value, self.scheduler.pc()))
			

EHCI_CMD = 0
EHCI_STATUS = 4
EHCI_INTR = 8
EHCI_FRAME_INDEX = 0xC
EHCI_CTRL_SEGMENT = 0x10
EHCI_PER_LIST_BASE = 0x14
EHCI_ASYNC_LIST_ADDR = 0x18
EHCI_CONFIG_FLAG = 0x40
		
EHCI0_START = 0xD040000
EHCI0_END = 0xD050000
EHCI1_START = 0xD120000
EHCI1_END = 0xD130000
EHCI2_START = 0xD140000
EHCI2_END = 0xD150000

class EHCIController:
	def __init__(self, scheduler, index):
		self.reset()
		self.scheduler = scheduler
		self.index = index
		
	def reset(self):
		self.cmd = 0
		self.status = 0x1000
		self.intr = 0
		self.frame_index = 0
		self.control_segment = 0
		self.per_list_addr = 0
		self.async_list_addr = 0
		self.config_flag = 0

	def read(self, addr):
		if addr == EHCI_CMD: return self.cmd
		elif addr == EHCI_STATUS: return self.status
		elif addr == EHCI_INTR: return self.intr
		elif addr == EHCI_FRAME_INDEX: return self.frame_index
		elif addr == EHCI_CTRL_SEGMENT: return self.control_segment

		print("EHCI(%i) READ 0x%X at %08X" %(self.index, addr, self.scheduler.pc()))
		return 0
		
	def write(self, addr, value):
		if addr == EHCI_CMD:
			if value & 2:
				self.reset()
				value &= ~2
				
			if value & 1:
				self.execute()
				value &= ~1
			self.cmd = value
			
		elif addr == EHCI_STATUS: self.status &= ~value
		elif addr == EHCI_INTR: self.intr = value
		elif addr == EHCI_FRAME_INDEX: self.frame_index = value
		elif addr == EHCI_PER_LIST_BASE: self.per_list_addr = value
		elif addr == EHCI_ASYNC_LIST_ADDR: self.async_list_addr = value
		elif addr == EHCI_CONFIG_FLAG: self.config_flag = value
				
		else:
			print("EHCI(%i) WRITE 0x%X %08X at %08X" %(self.index, addr, value, self.scheduler.pc()))
			
	def execute(self):
		print("EHCI(%i) EXECUTE at %08X" %(self.index, self.scheduler.pc()))
		
		
class USBDevice:

	device_descriptor = b"\x12\x01\x03\x10\x01\x00\x00\x10\x7E\x05\x05\x03\x09\x99\x01\x02\x03\x01"
	config_descriptor = b"\x09\x02\x19\x00\x01\x01\x04\x00\x32"
	interface_descriptor = b"\x09\x04\x00\x00\x00\x00\x00\x00\x05"
	endpoint_descriptor = b"\x07\x05\x81\x00\x00\x20\x04"

	def __init__(self):
		self.data = b""
		self.config_value = 0

	def handle(self, data):
		type, request, value, index, length = \
			struct.unpack("<BBHHH", data)
		if request == 5:
			print("USB SET_ADDRESS 0x%X" %value)
		elif request == 6:
			print("USB GET_DESCRIPTOR 0x%X 0x%X 0x%X" %(value, index, length))
			desc_type = value >> 8
			if desc_type == 1: #DEVICE
				self.data = self.device_descriptor
			elif desc_type == 2: #CONFIGURATION
				self.data = self.config_descriptor + self.interface_descriptor + self.endpoint_descriptor
		elif request == 9:
			print("USB SET_CONFIGURATION 0x%X" %value)
			self.config_value = value
		else:
			print("USB REQUEST", type, request, value, index, length)
			
	def send(self, data):
		print("USB DATA 0x%X" %len(data))
			
	def receive(self, size):
		return self.data[:size]
		
		
OHCI00_START = 0xD050000
OHCI00_END = 0xD060000
OHCI01_START = 0xD060000
OHCI01_END = 0xD070000
OHCI1_START = 0xD130000
OHCI1_END = 0xD140000
OHCI2_START = 0xD150000
OHCI2_END = 0xD160000

OHCI_REVISION = 0
OHCI_CONTROL = 4
OHCI_CMD_STATUS = 8
OHCI_INT_STATUS = 0xC
OHCI_INT_ENABLE = 0x10
OHCI_INT_DISABLE = 0x14
OHCI_HCCA = 0x18
OHCI_PERIOD_CURRENT_ED = 0x1C
OHCI_CONTROL_HEAD_ED = 0x20
OHCI_CONTROL_CURRENT_ED = 0x24
OHCI_BULK_HEAD_ED = 0x28
OHCI_BULK_CURRENT_ED = 0x2C
OHCI_DONE_HEAD = 0x30
OHCI_FM_INTERVAL = 0x34
OHCI_FM_REMAINING = 0x38
OHCI_FM_NUMBER = 0x3C
OHCI_PERIODIC_START = 0x40
OHCI_LS_THRESHOLD = 0x44
OHCI_RH_DESCRIPTOR_A = 0x48
OHCI_RH_DESCRIPTOR_B = 0x4C
OHCI_RH_STATUS = 0x50
OHCI_RH_PORT_STATUS = 0x54

class OHCIController:

	num_ports = 1

	def __init__(self, scheduler, armirq, physmem, index):
		self.scheduler = scheduler
		self.armirq = armirq
		self.physmem = physmem
		self.index = index
		self.reset()
		
		self.device = USBDevice()
		
	def reset(self):
		self.control = 0
		self.int_status = 0
		self.int_enable = 0
		self.hcca = 0
		self.done_head = 0
		self.frame_interval = 0
		self.periodic_start = 0
		self.descriptor_a = (1 << 24) | self.num_ports
		self.descriptor_b = 0
		
		self.control_head_ed = 0
		self.bulk_head_ed = 0
		
		self.port_enable = [0] * self.num_ports
		self.port_suspend = [0] * self.num_ports
		self.port_power = [0] * self.num_ports
		self.port_reset_change = [0] * self.num_ports
		self.port_enable_change = [0] * self.num_ports
		self.port_suspend_change = [0] * self.num_ports
		self.port_reset_change = [0] * self.num_ports

	def read(self, addr):
		if addr == OHCI_REVISION: return 0x10
		elif addr == OHCI_CONTROL: return self.control
		elif addr == OHCI_CMD_STATUS: return 0
		elif addr == OHCI_INT_STATUS: return self.int_status
		elif addr == OHCI_INT_ENABLE: return self.int_enable
		elif addr == OHCI_CONTROL_HEAD_ED: return self.control_head_ed
		elif addr == OHCI_RH_DESCRIPTOR_A: return self.descriptor_a
		elif addr == OHCI_RH_DESCRIPTOR_B: return self.descriptor_b
		elif addr == OHCI_RH_STATUS: return 0
		elif OHCI_RH_PORT_STATUS <= addr < OHCI_RH_PORT_STATUS + self.num_ports * 4:
			i = (addr - OHCI_RH_PORT_STATUS) // 4
			return 1 | (self.port_enable[i] << 1) | (self.port_suspend[i] << 2) | (self.port_power[i] << 8) | \
				self.port_reset_change[i] << 20
	
		print("OHCI(%i) READ 0x%X at %08X" %(self.index, addr, self.scheduler.pc()))
		return 0
		
	def write(self, addr, value):
		if addr == OHCI_CONTROL: self.control = value
		elif addr == OHCI_CMD_STATUS:
			if value & 1: self.reset()
			if value & 2: self.process_control()
			if value & 4: self.process_bulk()
		elif addr == OHCI_INT_STATUS: self.int_status &= ~value
		elif addr == OHCI_INT_ENABLE: self.int_enable |= value
		elif addr == OHCI_INT_DISABLE: self.int_enable &= ~value
		elif addr == OHCI_HCCA: self.hcca = value
		elif addr == OHCI_CONTROL_HEAD_ED: self.control_head_ed = value
		elif addr == OHCI_BULK_HEAD_ED: self.bulk_head_ed = value
		elif addr == OHCI_FM_INTERVAL: self.frame_interval = value
		elif addr == OHCI_PERIODIC_START: self.periodic_start = value
		elif addr == OHCI_RH_DESCRIPTOR_A: self.descriptor_a = value
		elif addr == OHCI_RH_DESCRIPTOR_B: self.descriptor_b = value
		elif addr == OHCI_RH_STATUS:
			if value & 1: self.port_power = [0] * self.num_ports
			if value & 0x10000: self.port_power = [1] * self.num_ports
		elif OHCI_RH_PORT_STATUS <= addr < OHCI_RH_PORT_STATUS + self.num_ports * 4:
			i = (addr - OHCI_RH_PORT_STATUS) // 4
			if value & 1: self.port_enable[i] = 0; self.port_enable_change[i] = 1
			if value & 2: self.port_enable[i] = 1; self.port_enable_change[i] = 1
			if value & 4: self.port_suspend[i] = 1; self.port_suspend_change[i] = 1
			if value & 8: self.port_suspend[i] = 0; self.port_suspend_change[i] = 1
			if value & 0x10: self.port_reset_change[i] = 1
			if value & 0x100: self.port_power[i] = 1
			if value & 0x200: self.port_power[i] = 0
			if value & 0x20000: self.port_enable_change[i] = 0
			if value & 0x40000: self.port_suspend_change[i] = 0
			if value & 0x100000: self.port_reset_change[i] = 0
		else:
			print("OHCI(%i) WRITE 0x%X %08X at %08X" %(self.index, addr, value, self.scheduler.pc()))
			
	def trigger_irq(self, flag):
		assert self.index == 1
		assert self.int_enable & flag
		self.int_status |= flag
		if self.index == 1:
			self.armirq.trigger_irq_all(6)
			
	def process_control(self):
		assert self.control & 0x10
		self.process_eds(self.control_head_ed)
		
	def process_bulk(self):
		assert self.control & 0x20
		self.process_eds(self.bulk_head_ed)
		
	def process_eds(self, current_ed):
		while current_ed:
			control, tail_td, current_td, next_ed = \
				struct.unpack("<IIII", self.physmem.read(current_ed, 0x10))
			tail_td &= ~0xF
			next_ed &= ~0xF
			if not control & 0x4000 and not current_td & 1: #Check skip and halt bits
				self.process_tds(current_ed, current_td & ~0xF, tail_td)
				
			current_ed = next_ed
			current_td &= ~0xF
				
		self.physmem.write(self.hcca + 0x84, struct.pack("<I", self.done_head))
		self.done_head = 0
		self.trigger_irq(2)
				
	def process_tds(self, base_ed, current, tail):
		while current != tail:
			control, current_ptr, next, end_ptr = \
				struct.unpack("<IIII", self.physmem.read(current, 0x10))

			size = (end_ptr - current_ptr + 1) & 0xFFFFFFFF
			direction = (control >> 19) & 3
			if direction == 0:
				data = self.physmem.read(current_ptr, size)
				self.device.handle(data)
			elif direction == 1:
				data = self.physmem.read(current_ptr, size)
				self.device.send(data)
			elif direction == 2:
				data = self.device.receive(size)
				self.physmem.write(current_ptr, data)
			else:
				raise ValueError("OHCI(%i): DIR=RESERVED" %self.index)

			#Set condition code
			control = control & ~0xF0000000 #No error
			self.physmem.write(current, struct.pack("<I", control))
				
			#Remove from ed
			dword = struct.unpack("<I", self.physmem.read(base_ed + 8, 4))[0]
			dword = (dword & 0xF) | next
			self.physmem.write(base_ed + 8, struct.pack("<I", dword))
				
			#Add to done queue
			self.physmem.write(current + 8, struct.pack("<I", self.done_head))
			self.done_head = current
			current = next


AHCI_HBA_CONTROL = 0xD160404
AHCI_HBA_INT_STATUS = 0xD160408
			
AHCI_CMD_BASE = 0xD160500
AHCI_CMD_BASE_HI = 0xD160504
AHCI_FIS_BASE = 0xD160508
AHCI_FIS_BASE_HI = 0xD16050C
AHCI_INT_STATUS = 0xD160510
AHCI_INT_ENABLE = 0xD160514
AHCI_CMD_STATUS = 0xD160518
AHCI_TASK_FILE_DATA = 0xD160520
AHCI_STATUS = 0xD160528
AHCI_CONTROL = 0xD16052C
AHCI_ERROR = 0xD160530
AHCI_CMD_ISSUE = 0xD160538

SATA_INT_STATE = 0xD160800
SATA_INT_MASK = 0xD160804

AHCI_START = 0xD160000
AHCI_END = 0xD170000

FIS_TYPE_REG_H2D = 0x27

ATAPI_REQUEST_SENSE = 3
ATAPI_CF_AUTHENTICATE = 0xF1
ATAPI_CF_START_STOP_UNIT_CMD = 0xF2
ATAPI_CF_READ_CMD = 0xF3
ATAPI_CF_INQUIRY = 0xF5

class PRDTEntry:
	def __init__(self, physmem, data):
		self.physmem = physmem
		dwords = struct.unpack("<4I", data)
		self.data_addr = dwords[0]
		self.interrupt = dwords[3] >> 31
		self.byte_count = (dwords[3] & 0x3FFFFF) + 1
		
	def write(self, data):
		if len(data) > self.byte_count:
			raise RuntimeError("PRDT write overflow")
		self.physmem.write(self.data_addr, data)
		
class AHCICmdList:
	def __init__(self, physmem, addr):
		self.physmem = physmem
		self.addr = addr

		data = physmem.read(addr, 0x20)
		prdt_count = struct.unpack_from("<H", data, 2)[0]
		table_addr = struct.unpack_from("<I", data, 8)[0]
		
		self.fis = physmem.read(table_addr, 0x40)
		self.atapi = physmem.read(table_addr + 0x40, 0x10)

		prdt_data = physmem.read(table_addr + 0x80, prdt_count * 0x10)
		self.prdts = [PRDTEntry(physmem, prdt_data[i * 0x10 : (i + 1) * 0x10]) for i in range(prdt_count)]
		
		self.byte_count = 0
	
	def add_bytes(self, num):
		self.byte_count += num
		self.physmem.write(self.addr + 4, struct.pack("<I", self.byte_count))
	
	def write_prdt(self, index, data):
		self.prdts[index].write(data)
		self.add_bytes(len(data))
		
	def fill_prdts(self):
		for i, prdt in enumerate(self.prdts):
			self.write_prdt(i, b"\0" * prdt.byte_count)
			
class AHCIController:
	def __init__(self, scheduler, armirq, physmem):
		self.scheduler = scheduler
		self.armirq = armirq
		self.physmem = physmem
	
		self.hba_control = 0
		self.hba_int_status = 0
	
		self.cmd_base = 0
		self.fis_base = 0
		self.int_status = 0
		self.int_enable = 0
		self.cmd_status = 0
		self.status = 0x3
		
		self.sata_int_state = 0
		self.sata_int_mask = 0
		
	def read(self, addr):
		if addr == AHCI_HBA_CONTROL: return self.hba_control
		elif addr == AHCI_HBA_INT_STATUS: return self.hba_int_status
	
		elif addr == AHCI_CMD_BASE: return self.cmd_base
		elif addr == AHCI_CMD_BASE_HI: return 0
		elif addr == AHCI_FIS_BASE: return self.fis_base
		elif addr == AHCI_FIS_BASE_HI: return 0
		elif addr == AHCI_INT_STATUS: return self.int_status
		elif addr == AHCI_INT_ENABLE: return self.int_enable
		elif addr == AHCI_CMD_STATUS: return self.cmd_status
		elif addr == AHCI_TASK_FILE_DATA: return 0
		elif addr == AHCI_STATUS: return self.status
		elif addr == AHCI_ERROR: return 0
		elif addr == AHCI_CMD_ISSUE: return 0
		
		elif addr == SATA_INT_STATE: return self.sata_int_state
		elif addr == SATA_INT_MASK: return self.sata_int_mask
		
		print("AHCI READ 0x%X at %08X" %(addr, self.scheduler.pc()))
		return 0
		
	def write(self, addr, value):
		if addr == AHCI_HBA_CONTROL: self.hba_control = value & ~1
		elif addr == AHCI_HBA_INT_STATUS: self.hba_int_status &= ~value
	
		elif addr == AHCI_CMD_BASE: self.cmd_base = value & ~0x3FF
		elif addr == AHCI_CMD_BASE_HI: pass
		elif addr == AHCI_FIS_BASE: self.fis_base = value & ~0xFF
		elif addr == AHCI_FIS_BASE_HI: pass
		elif addr == AHCI_INT_STATUS: self.int_status &= ~value
		elif addr == AHCI_INT_ENABLE: self.int_enable = value
		elif addr == AHCI_CMD_STATUS: self.cmd_status = value
		elif addr == AHCI_CONTROL: pass
		elif addr == AHCI_ERROR: pass
		elif addr == AHCI_CMD_ISSUE:
			for i in range(32):
				if value & (1 << i):
					self.issue_cmd(i)
					
		elif addr == SATA_INT_STATE: self.sata_int_state = value
		elif addr == SATA_INT_MASK: self.sata_int_mask = value
		else:
			print("AHCI WRITE 0x%X %08X at %08X" %(addr, value, self.scheduler.pc()))
			
	def issue_cmd(self, index):
		addr = self.cmd_base + index * 0x20
		self.handle_cmd_table(AHCICmdList(self.physmem, addr))
		
		self.hba_int_status = 1
		self.sata_int_status = self.sata_int_mask
		self.armirq.trigger_irq_all(28)
		self.armirq.trigger_irq_lt(6)
		
	def handle_cmd_table(self, cmd_list):
		if cmd_list.fis[0] == FIS_TYPE_REG_H2D:
			command = cmd_list.fis[2]
			if command == 0xA0: #PACKET
				self.handle_atapi(cmd_list)
			else:
				print("FIS COMMAND 0x%X at %08X" %(command, self.scheduler.pc()))
		else:
			print("AHCI COMMAND %i at %08X" %(data[0], self.scheduler.pc()))
		
	def handle_atapi(self, cmd_list):
		command = cmd_list.atapi[0]
		if command == ATAPI_REQUEST_SENSE:
			cmd_list.write_prdt(0, b"\xF0" + bytes(17) + b"\x02" + bytes(13))
			
		elif command == ATAPI_CF_INQUIRY:
			cmd_list.write_prdt(0, b"\0\0\0\x05" + b"\0" * 28)

		else:
			print("ATAPI CMD 0x%X at %08X" %(command, self.scheduler.pc()))
			cmd_list.fill_prdts()

		
SDIO0_START = 0xD070000
SDIO0_END = 0xD080000
SDIO1_START = 0xD100000
SDIO1_END = 0xD110000
SDIO2_START = 0xD110000
SDIO2_END = 0xD120000
WIFI_START = 0xD080000
WIFI_END = 0xD090000
		
class SDIOController:

	TYPE_SD = 0
	TYPE_SDIO = 1
	TYPE_MMC = 2
	TYPE_UNK = 3
	
	STATE_IDLE = 0
	STATE_READY = 1
	STATE_IDENT = 2
	STATE_STBY = 3
	STATE_TRAN = 4
	STATE_DATA = 5
	STATE_RCV = 6
	STATE_PRG = 7
	STATE_DIS = 8

	capabilities = 0
	voltage_range = 0x300000
	cis_pointer = 0x1000
	
	card_info = b"\x22\x04\x00\xFF\xFF\x32" #CISTPL_FUNCE
	card_info += b"\xFF\x00" #CISTPL_END
	
	# V1:
	#	[0x00XXXX32, 0x5B5XPQXX, 0xXXXXXXXX, 0xRXS0XTXU]
	# V2:
	#	[0x400E0032, 0x5B59V0QX, 0xXXXX7F00, 0x0A20X0XU]
	# P: 1XXX
	# Q: 00XX
	# R: X00X
	# S: XXX0
	# T: XX00
	# U: XXX1
	# V: 000X
	csd = [0x00000032, 0x5B508000, 0x00000000, 0x00000001] #V1
	csd = [0x400E0032, 0x5B590000, 0xFFFF7F80, 0x0A400001] #V2
	
	def __init__(self, scheduler, armirq, physmem, index, type):
		self.file = None
		if index == 1:
			self.file = open("D:/WiiU/MLC/mlc_work.bin", "r+b")
			#self.file = open("D:/WiiU/MLC/mlc.bin", "rb")
			#self.file.write = lambda *args: None
	
		self.scheduler = scheduler
		self.armirq = armirq
		self.physmem = physmem
		self.index = index
		self.type = type
		
		self.dma_addr = 0
		self.block_count = 0
		self.block_size = 0
		self.argument = 0
		self.transfer_mode = 0
		self.command = 0
		self.result0 = 0
		self.result1 = 0
		self.result2 = 0
		self.result3 = 0
		self.host_control = 0
		self.power_control = 0
		self.block_gap_control = 0
		self.wakeup_control = 0
		self.clock_control = 0
		self.timeout_control = 0
		self.int_status = 0
		self.error_status = 0
		
		self.block_length = 0
		
		self.bus_width = 0
		self.cd_disable = 0
		
		self.app_cmd = False
		self.state = self.STATE_IDLE
		
	def close(self):
		if self.file:
			self.file.close()
		
	def trigger_interrupt(self):
		if self.index == 0:
			self.armirq.trigger_irq_all(7)
		else:
			self.armirq.trigger_irq_lt(0)
	
	def read(self, addr):
		if addr == 0xC:
			return self.transfer_mode | (self.command << 16)
		elif addr == 0x10: return self.result0
		elif addr == 0x14: return self.result1
		elif addr == 0x18: return self.result2
		elif addr == 0x1C: return self.result3
		elif addr == 0x24: return 0x80000 #Write enabled
		elif addr == 0x28:
			return self.host_control | (self.power_control << 8) | \
				(self.block_gap_control << 16) | (self.wakeup_control << 24)
		elif addr == 0x2C:
			return self.clock_control | (self.timeout_control << 16)
		elif addr == 0x30: return self.int_status | (self.error_status << 16)
		elif addr == 0x40: return self.capabilities
		
		print("SDIO(%i) READ 0x%X at %08X" %(self.index, addr, self.scheduler.pc()))
		return 0
		
	def write(self, addr, value):
		if addr == 0: self.dma_addr = value
		elif addr == 4:
			self.block_count = value >> 16
			self.block_size = value & 0xFFF
		elif addr == 8: self.argument = value
		elif addr == 0xC:
			self.command = value >> 16
			self.transfer_mode = value & 0xFFFF
			
			self.int_status = 3
			self.error_status = 0
			if self.app_cmd:
				self.handle_app_command(value >> 24, self.argument)
			else:
				self.handle_command(value >> 24, self.argument)

			self.trigger_interrupt()
		elif addr == 0x28:
			self.host_control = value & 0xFF
			self.power_control = (value >> 8) & 0xFF
			self.block_gap_control = (value >> 16) & 0xFF
			self.wakeup_control = value >> 24
		elif addr == 0x2C:
			self.clock_control = value & 0xFFFF
			if self.clock_control & 1:
				self.clock_control |= 2
			self.timeout_control = (value >> 16) & 0xFF
		elif addr == 0x30:
			self.int_status &= ~(value & 0xFF)
			self.error_status &= ~(value >> 16)
		else:
			print("SDIO(%i) WRITE 0x%X %08X at %08X" %(self.index, addr, value, self.scheduler.pc()))
			
	def get_card_status(self):
		#Ready for data
		return (self.state << 9) | 0x100 | (self.app_cmd << 5)
			
	def handle_app_command(self, cmd, arg):
		if cmd == 6: #Set bus width
			self.bus_width = arg & 3
			self.result0 = self.get_card_status()
		elif cmd == 41: #Voltage range
			if self.type == self.TYPE_SD:
				self.result0 = self.voltage_range
				if arg & self.voltage_range:
					self.result0 |= 0xC0000000 #Ready to operate | SDHC/SDXC
			else:
				self.int_status |= 0x8000
		else:
			print("SDIO(%i) ACMD %i %08X at %08X" %(self.index, cmd, arg, self.scheduler.pc()))

		self.app_cmd = False
		
	def handle_command(self, cmd, arg):
		self.result0 = 0
		self.result1 = 0
		self.result2 = 0
		self.result3 = 0
	
		if cmd == 0: pass #Reset
		elif cmd == 1: #Voltage range
			if self.type == self.TYPE_MMC:
				self.result0 = self.voltage_range
				if arg & self.voltage_range:
					self.result0 |= 0x80000000 #Ready to operate
			else:
				self.int_status |= 0x8000
		elif cmd == 2: #CID register
			pass
		elif cmd == 3:
			self.result0 = 0x400 #Relative card address
		elif cmd == 5: #Voltage range
			if self.type == self.TYPE_SDIO:
				self.result0 = self.voltage_range | 0x8000000 #Memory present
				if arg & self.voltage_range:
					self.result0 |= 0x80000000 #Ready to operate
			else:
				self.int_status |= 0x8000
		elif cmd == 7: #Card select
			self.state = self.STATE_STBY
			self.result0 = self.get_card_status()
		elif cmd == 8: #Voltage and check pattern
			self.result0 = arg & 0xFFF
		elif cmd == 9: #CSD register
			self.result3 = ((self.csd[0] >> 8) | (self.csd[3] << 24)) & 0xFFFFFFFF
			self.result2 = ((self.csd[1] >> 8) | (self.csd[0] << 24)) & 0xFFFFFFFF
			self.result1 = ((self.csd[2] >> 8) | (self.csd[1] << 24)) & 0xFFFFFFFF
			self.result0 = ((self.csd[3] >> 8) | (self.csd[2] << 24)) & 0xFFFFFFFF
		elif cmd == 13: #Send card status
			self.state = self.STATE_TRAN
			self.result0 = self.get_card_status()
		elif cmd == 16: #Set block len
			self.block_length = arg
			self.result0 = self.get_card_status()
		elif cmd == 17: #Read single block
			assert self.block_count == 1
			self.file.seek(self.argument << 9)
			self.physmem.write(self.dma_addr, self.file.read(self.block_count * self.block_size))
		elif cmd == 18: #Read multiple block
			self.file.seek(self.argument << 9)
			self.physmem.write(self.dma_addr, self.file.read(self.block_count * self.block_size))
		elif cmd == 25: #Write multiple block
			data = self.physmem.read(self.dma_addr, self.block_count * self.block_size)
			self.file.seek(self.argument << 9)
			self.file.write(data)
		elif cmd == 52: #Read/write direct
			function = (arg >> 28) & 7
			if function != 0:
				raise NotImplementedError("SDIO(%i) CMD52 function %i at %08X" %(self.index, function, self.scheduler.pc()))

			reg_addr = (arg >> 9) & 0x1FFFF
			if arg & 0x80000000: #Write
				self.result0 = arg & 0xFF
				self.write_register(reg_addr, self.result0)
				if arg & 0x8000000:
					self.result0 = self.read_register(reg_addr)
			else:
				self.result0 = self.read_register(reg_addr)
		elif cmd == 55: #App cmd
			self.result0 = 0x20
			self.app_cmd = True
		else:
			print("SDIO(%i) CMD %i %08X at %08X" %(self.index, cmd, arg, self.scheduler.pc()))
		
	def read_register(self, addr):
		if addr == 7: return self.bus_width | (self.cd_disable << 7)
		elif addr == 9: return self.cis_pointer & 0xFF
		elif addr == 0xA: return (self.cis_pointer >> 8) & 0xFF
		elif addr == 0xB: return self.cis_pointer >> 16
		elif addr == 0x13: return 1
		elif addr >= 0x1000: return self.card_info[addr - 0x1000]
	
		print("SDIO(%i) READ REG 0x%X at %08X" %(self.index, addr, self.scheduler.pc()))
		return 0
		
	def write_register(self, addr, value):
		if addr == 6: pass #Reset, i/o abort
		elif addr == 7:
			self.bus_width = value & 3
			self.cd_disable = value >> 7
		else:
			print("SDIO(%i) WRITE REG 0x%X 0x%02X at %08X" %(self.index, addr, value, self.scheduler.pc()))

		
NAND_CTRL = 0
NAND_CONFIG = 4
NAND_ADDR1 = 8
NAND_ADDR2 = 0xC
NAND_DATABUF = 0x10
NAND_ECCBUF = 0x14

class NANDBank:

	#0xECDC: Samsung K9XXG08XXB
	#0xADDC: Hynix HY27UF084G2B
	#0x98DC: Toshiba TC58NVG2S3ETA00
	#0x01DC: Spansion Undetermined
	chip_id = 0xECDC

	def __init__(self, scheduler, armirq, physmem, slc, slcspare, slccmpt, slccmptspare):
		self.scheduler = scheduler
		self.armirq = armirq
		self.physmem = physmem
		self.slc = slc
		self.slcspare = slcspare
		self.slccmpt = slccmpt
		self.slccmptspare = slccmptspare

		self.file = self.slccmpt
		self.filespare = self.slccmptspare
		self.reset()
		
	def reset(self):
		self.control = 0
		self.config = 0
		self.addr1 = 0
		self.addr2 = 0
		self.databuf = 0
		self.eccbuf = 0
		
	def set_bank(self, cmpt):
		if cmpt:
			self.file = self.slccmpt
			self.filespare = self.slccmptspare
		else:
			self.file = self.slc
			self.filespare = self.slcspare
		
	def read(self, addr):
		if addr == NAND_CTRL: return self.control
		elif addr == NAND_CONFIG: return self.config
		elif addr == NAND_ADDR1: return self.addr1
		elif addr == NAND_ADDR2: return self.addr2
		elif addr == NAND_DATABUF: return self.databuf
		elif addr == NAND_ECCBUF: return self.eccbuf
		
	def write(self, addr, value):
		if addr == NAND_CTRL: self.control = self.start_command(value)
		elif addr == NAND_CONFIG:
			self.config = value
		elif addr == NAND_ADDR1: self.addr1 = value
		elif addr == NAND_ADDR2: self.addr2 = value
		elif addr == NAND_DATABUF: self.databuf = value
		elif addr == NAND_ECCBUF: self.eccbuf = value
		
	def start_command(self, value):
		if value & 0x80000000:
			command = (value >> 16) & 0xFF
			write = (value >> 14) & 1
			read = (value >> 13) & 1
			length = value & 0xFFF
			self.handle_command(command, write, read, length)

			if value & 0x40000000:
				self.armirq.trigger_irq_all(1)
			return value & ~0x80000000
		else:
			self.reset()
			return 0

	def handle_command(self, command, write, read, length):
		if command == 0: #Init read
			assert not read and not write and not length
			
		elif command == 0x10: #Finish write
			assert not read and not write and not length
			
		elif command == 0x30: #Read
			assert read and not write and (length == 0x840 or length == 0x40)
			if length == 0x840:
				self.file.seek((self.addr2 << 11) | self.addr1)
				self.physmem.write(self.databuf, self.file.read(0x800))

				self.filespare.seek(self.addr2 << 6)
				sparedata = self.filespare.read(0x40)
				self.physmem.write(self.eccbuf, sparedata)
				self.physmem.write(self.eccbuf ^ 0x40, sparedata[0x30:])
			else:
				self.filespare.seek(self.addr2 << 6)
				self.physmem.write(self.databuf, self.filespare.read(0x40))
				
		elif command == 0x60: #Erase init 1
			assert not read and not write and not length
			
		elif command == 0x70: #Erase
			assert read and not write and length == 0x40
			self.physmem.write(self.databuf, b"\0" * length)
			
		elif command == 0x80: #Write
			assert not read and write and length == 0x800
			data = self.physmem.read(self.databuf, length)
			self.file.seek((self.addr2 << 11) | self.addr1)
			self.file.write(data)
			self.next_spare = self.addr2 << 6
			
		elif command == 0x85: #Write spare
			assert not read and write and length == 0x40
			data = self.physmem.read(self.databuf, length)
			self.filespare.seek(self.next_spare)
			self.filespare.write(data)

		elif command == 0x90: #Get chip id
			assert read and not write and length == 0x20
			self.physmem.write(self.databuf, struct.pack(">H", self.chip_id))
		
		elif command == 0xD0: #Erase init 2
			assert not read and not write and not length
			
		elif command == 0xFF: #Reset
			assert not read and not write and not length
			
		else:
			print("NAND COMMAND 0x%X %i %i 0x%X at %08X" %(command, write, read, length, self.scheduler.pc()))
		
NAND_BANK = 0xD010018
NAND_BANK_CTRL = 0xD010030
NAND_INT_MASK = 0xD010034

NAND_MAIN_START = 0xD010000
NAND_MAIN_END = 0xD010018
NAND_BANKS_START = 0xD010040
NAND_BANKS_END = 0xD010100
		
NAND_START = 0xD010000
NAND_END = 0xD020000

class NANDController:
	def __init__(self, scheduler, armirq, physmem):
		self.scheduler = scheduler

		self.slc = open("slc_work.bin", "r+b")
		self.slcspare = open("slcspare_work.bin", "r+b")

		self.slccmpt = open("slccmpt_work.bin", "r+b")
		self.slccmptspare = open("slccmptspare_work.bin", "r+b")
		
		self.armirq = armirq
		self.physmem = physmem
		
		self.main_bank = self.create_bank()
		self.banks = [self.create_bank() for i in range(8)]
		
		self.reset()

	def create_bank(self):
		return NANDBank(self.scheduler, self.armirq, self.physmem, self.slc, self.slcspare, self.slccmpt, self.slccmptspare)
		
	def close(self):
		self.slc.close()
		self.slcspare.close()
		self.slccmpt.close()
		self.slccmptspare.close()
	
	def reset(self):
		self.main_bank.reset()
		for bank in self.banks:
			bank.reset()

		self.bank = 0
		self.bank_control = 0
		self.int_mask = 0

	def read(self, addr):
		if NAND_MAIN_START <= addr < NAND_MAIN_END: return self.main_bank.read(addr - NAND_MAIN_START)
		elif addr == NAND_BANK: return self.bank
		elif addr == NAND_BANK_CTRL: return self.bank_control
		elif addr == NAND_INT_MASK: return self.int_mask
		elif NAND_BANKS_START <= addr < NAND_BANKS_END:
			return self.banks[(addr - NAND_BANKS_START) // 0x18].read((addr - NAND_BANKS_START) % 0x18)
	
		print("NAND READ 0x%X at %08X" %(addr, self.scheduler.pc()))
		return 0
		
	def write(self, addr, value):
		if NAND_MAIN_START <= addr < NAND_MAIN_END: self.main_bank.write(addr - NAND_MAIN_START, value)
		elif addr == NAND_BANK:
			self.main_bank.set_bank(not value & 2)
			for bank in self.banks:
				bank.set_bank(not value & 2)
			self.bank = value
		elif addr == NAND_BANK_CTRL:
			if value & 0x80000000:
				banks = (value >> 16) & 0xFF
				assert (1 << banks) - 1 == value & 0xFF
				for i in range(banks):
					bank = self.banks[i]
					bank.config = bank.start_command(bank.config)
				self.int_mask &= ~(value & 0xFF)
				self.bank_control = value & ~0x80000000
				self.armirq.trigger_irq_all(1)
		elif NAND_BANKS_START <= addr < NAND_BANKS_END:
			self.banks[(addr - NAND_BANKS_START) // 0x18].write((addr - NAND_BANKS_END) % 0x18, value)
		else:
			print("NAND WRITE 0x%X %08X at %08X" %(addr, value, self.scheduler.pc()))

			
AES_CTRL = 0
AES_SRC = 4
AES_DEST = 8
AES_KEY = 0xC
AES_IV = 0x10

AES_START = 0xD020000
AES_END = 0xD030000
AESS_START = 0xD180000
AESS_END = 0xD190000

class AESController:
	def __init__(self, scheduler, armirq, physmem, index):
		self.scheduler = scheduler
		self.armirq = armirq
		self.physmem = physmem
		self.index = index
		self.reset()
		
	def reset(self):
		self.ctrl = 0
		self.src = 0
		self.dest = 0
		self.key = b"\0" * 16
		self.iv = b"\0" * 16	

	def read(self, addr):
		if addr == AES_CTRL: return self.ctrl
		elif addr == AES_SRC: return self.src
		elif addr == AES_DEST: return self.dest
	
		print("AES READ 0x%X at %08X" %(addr, self.scheduler.pc()))
		return 0

	def write(self, addr, value):
		if addr == AES_CTRL:
			if value & 0x80000000:
				self.ctrl = (value & ~0x80000000) | 0xFFF
				
				data = self.physmem.read(self.src, ((value & 0xFFF) + 1) * 16)
				if value & 0x10000000:
					if value & 0x1000:
						raise NotImplementedError("AES block chain continue")

					cipher = AES.new(self.key, AES.MODE_CBC, self.iv)
					if value & 0x8000000:
						data = cipher.decrypt(data)
					else:
						data = cipher.encrypt(data)

				self.physmem.write(self.dest, data)

				if value & 0x40000000:
					self.trigger_interrupt()

			else:
				self.reset()

		elif addr == AES_SRC: self.src = value
		elif addr == AES_DEST: self.dest = value
		elif addr == AES_KEY:
			self.key = self.key[4:] + struct.pack(">I", value)
		elif addr == AES_IV:
			self.iv = self.iv[4:] + struct.pack(">I", value)
		else:
			print("AES WRITE 0x%X %08X at %08X" %(addr, value, self.scheduler.pc()))
			
	def trigger_interrupt(self):
		if self.index == 0: self.armirq.trigger_irq_all(2)
		else: self.armirq.trigger_irq_lt(8)
			

SHA_CTRL = 0
SHA_SRC = 4
SHA_H0 = 8
SHA_H1 = 0xC
SHA_H2 = 0x10
SHA_H3 = 0x14
SHA_H4 = 0x18
			
SHA_START = 0xD030000
SHA_END = 0xD040000
SHAS_START = 0xD190000
SHAS_END = 0xD1A0000

class SHAController:
	def __init__(self, scheduler, armirq, physmem, index):
		self.scheduler = scheduler
		self.armirq = armirq
		self.physmem = physmem
		self.index = index
		self.reset()
		
	def reset(self):
		self.sha1 = pyemu.SHA1()
		self.control = 0
		self.srcaddr = 0

	def read(self, addr):
		if addr == SHA_CTRL: return self.control
		elif addr == SHA_SRC: return self.srcaddr
		elif addr == SHA_H0: return self.sha1.h0
		elif addr == SHA_H1: return self.sha1.h1
		elif addr == SHA_H2: return self.sha1.h2
		elif addr == SHA_H3: return self.sha1.h3
		elif addr == SHA_H4: return self.sha1.h4
	
		print("SHA READ 0x%X at %08X" %(addr, self.scheduler.pc()))
		return 0
		
	def write(self, addr, value):
		if addr == SHA_CTRL:
			if value & 0x80000000:
				blocks = (value & 0x3FF) + 1
				for i in range(blocks):
					data = self.physmem.read(self.srcaddr, 0x40)
					self.srcaddr += 0x40
					self.sha1.process_block(data)

				if value & 0x40000000:
					self.trigger_interrupt()
				self.control = value & ~0x80000000
			else:
				self.reset()
		elif addr == SHA_SRC: self.srcaddr = value
		elif addr == SHA_H0: self.sha1.h0 = value
		elif addr == SHA_H1: self.sha1.h1 = value
		elif addr == SHA_H2: self.sha1.h2 = value
		elif addr == SHA_H3: self.sha1.h3 = value
		elif addr == SHA_H4: self.sha1.h4 = value
		else:
			print("SHA WRITE 0x%X %08X at %08X" %(addr, value, self.scheduler.pc()))
			
	def trigger_interrupt(self):
		if self.index == 0: self.armirq.trigger_irq_all(3)
		else: self.armirq.trigger_irq_lt(9)
			
			
LATTE_START = 0xD000000
LATTE_END = 0xD001000
			
LT_TIMER = 0xD000010
LT_ALARM = 0xD000014
LT_AHB_WDG_CONFIG = 0xD00004C
LT_AHB_DMA_STATUS = 0xD000050
LT_AHB_CPU_STATUS = 0xD000054
LT_ERROR = 0xD000058
LT_ERROR_MASK = 0xD00005C
LT_MEMIRR = 0xD000060
LT_AHBPROT = 0xD000064
LT_EXICTRL = 0xD000070
LT_D800074 = 0xD000074
LT_D800088 = 0xD000088
LT_D800180 = 0xD000180
LT_COMPAT_MEMCTRL_WORKAROUND = 0xD000188
LT_BOOT0 = 0xD00018C
LT_CLOCKINFO = 0xD000190
LT_RESETS_COMPAT = 0xD000194
LT_CLOCKGATE_COMPAT = 0xD000198
LT_D8001D0 = 0xD0001D0
LT_IOPOWER = 0xD0001DC
LT_IOSTRENGTH_CTRL0 = 0xD0001E0
LT_IOSTRENGTH_CTRL1 = 0xD0001E4
LT_ACRCLK_STRENGTH_CTRL = 0xD0001E8
LT_OTPCMD = 0xD0001EC
LT_OTPDATA = 0xD0001F0
LT_D800204 = 0xD000204
LT_ASICREV_ACR = 0xD000214
LT_INTSR_AHBALL_PPC0 = 0xD000440
LT_INTSR_AHBLT_PPC0 = 0xD000444
LT_INTMR_AHBALL_PPC0 = 0xD000448
LT_INTMR_AHBLT_PPC0 = 0xD00044C
LT_INTSR_AHBALL_PPC1 = 0xD000450
LT_INTSR_AHBLT_PPC1 = 0xD000454
LT_INTMR_AHBALL_PPC1 = 0xD000458
LT_INTMR_AHBLT_PPC1 = 0xD00045C
LT_INTSR_AHBALL_PPC2 = 0xD000460
LT_INTSR_AHBLT_PPC2 = 0xD000464
LT_INTMR_AHBALL_PPC2 = 0xD000468
LT_INTMR_AHBLT_PPC2 = 0xD00046C
LT_INTSR_AHBALL_ARM = 0xD000470
LT_INTSR_AHBLT_ARM = 0xD000474
LT_INTMR_AHBALL_ARM = 0xD000478
LT_INTMR_AHBLT_ARM = 0xD00047C
LT_INTMR_AHBALL_ARM2x = 0xD000480
LT_INTMR_AHBLT_ARM2x = 0xD000484
LT_AHB2_DMA_STATUS = 0xD0004A4
LT_AHB2_CPU_STATUS = 0xD0004A8
LT_D800500 = 0xD000500
LT_D800504 = 0xD000504
LT_OTPPROT = 0xD000510
LT_ASICREV_CCR = 0xD0005A0
LT_DEBUG = 0xD0005A4
LT_COMPAT_MEMCTRL_STATE = 0xD0005B0
LT_IOP2X = 0xD0005BC
LT_IOSTRENGTH_CTRL2 = 0xD0005C8
LT_D8005CC = 0xD0005CC
LT_RESETS = 0xD0005E0
LT_RESETS_AHMN = 0xD0005E4
LT_SYSPLL_CFG = 0xD0005EC
LT_ABIF_CPLTL_OFFSET = 0xD000620
LT_ABIF_CPLTL_DATA = 0xD000624
LT_D800628 = 0xD000628
LT_60XE_CFG = 0xD000640
LT_D800660 = 0xD000660

LT_GPIO_START = 0xD0000C0
LT_GPIO_END = 0xD000100

LT_IPC_PPC0_START = 0xD000400
LT_IPC_PPC0_END = 0xD000410
LT_IPC_PPC1_START = 0xD000410
LT_IPC_PPC1_END = 0xD000420
LT_IPC_PPC2_START = 0xD000420
LT_IPC_PPC2_END = 0xD000430

LT_IRQ_PPC0_START = 0xD000440
LT_IRQ_PPC0_END = 0xD000450
LT_IRQ_PPC1_START = 0xD000450
LT_IRQ_PPC1_END = 0xD000460
LT_IRQ_PPC2_START = 0xD000460
LT_IRQ_PPC2_END = 0xD000470
LT_IRQ_ARM_START = 0xD000470
LT_IRQ_ARM_END = 0xD000488

LT_GPIO2_START = 0xD000520
LT_GPIO2_END = 0xD000560

BUS_CLOCK_SPEED = 248625000
TIMER_SPEED = 0x1DA36E
HARDWARE_VERSION_ACR = 0x21
HARDWARE_VERSION_CCR = 0xCAFE0060
			

class OTPController:
	def __init__(self, scheduler):
		with open("otp.bin", "rb") as f:
			self.data = struct.unpack(">256I", f.read())
			
		self.scheduler = scheduler

	def read(self, bank, index):
		return self.data[bank * 0x20 + index]


class SEEPROMController:

	LISTEN = 0
	WRITE = 1
	POST_WRITE = 2
	POST_POST_WRITE = 3

	def __init__(self, scheduler):
		with open("seeprom.bin", "rb") as f:
			self.data = list(struct.unpack(">256H", f.read()))
			
		self.state = self.LISTEN
		
		self.scheduler = scheduler

	def init_transfer(self):
		self.index = 0
		self.pin_state = 0
		self.bits = 11
		self.offset = 0
		
	def update_pin(self):
		if self.bits:
			self.index = (self.index << 1) | self.pin_state
			self.bits -= 1
			if not self.bits:
				if self.state == self.LISTEN:
					self.handle_command(self.index)
				elif self.state == self.WRITE:
					self.handle_write(self.index)
				elif self.state == self.POST_WRITE:
					self.handle_write_done()
				else:
					self.state = self.LISTEN
		else:
			self.pin_state = (self.output >> (self.output_size - 1)) & 1
			self.output <<= 1
			
	def read_value(self, index):
		return self.data[index]
		
	def handle_command(self, value):
		if self.index & ~0xC0 == 0x400: pass #Control
		elif self.index & 0xF00 == 0x500: #Write
			self.bits = 16
			self.index = 0
			self.offset = self.index & 0xFF
			self.state = self.WRITE
		elif self.index & 0xF00 == 0x600: #Read
			self.output = self.read_value(self.index & 0xFF)
			self.output_size = 16
		else:
			print("SEEPROM CMD 0x%X at %08X" %(self.index, self.scheduler.pc()))
			
	def handle_write(self, value):
		self.data[self.offset] = value
		self.bits = 2
		self.state = self.POST_WRITE
		
	def handle_write_done(self):
		self.output = 1
		self.output_size = 1
		self.bits = 2
		self.state = self.POST_POST_WRITE

		
PIN_DWIFI_MODE = 1
PIN_ESP10_WORKAROUND = 5
PIN_9 = 9
PIN_EEPROM_CS = 10
PIN_EEPROM_SK = 11
PIN_EEPROM_DO = 12
PIN_EEPROM_DI = 13
PIN_AV0_I2C_CLOCK = 14
PIN_AV0_I2C_DATA = 15
PIN_AV1_I2C_CLOCK = 24
PIN_AV1_I2C_DATA = 25
PIN_BLUETOOTH_MODE = 27
PIN_WIFI_MODE = 29
PIN_31 = 31
		
class GPIOGroup1:
	def __init__(self, scheduler, gpio):
		self.seeprom = SEEPROMController(scheduler)
		
		self.scheduler = scheduler
		self.gpio = gpio
		
	def read(self, espresso):
		return self.seeprom.pin_state << PIN_EEPROM_DI
		
	def write(self, pin, state, espresso):
		if pin == PIN_DWIFI_MODE: pass
		elif pin == PIN_ESP10_WORKAROUND: pass
		elif pin == PIN_9: pass
		elif pin == PIN_EEPROM_CS:
			if state == 1:
				self.seeprom.init_transfer()
		elif pin == PIN_EEPROM_SK:
			if state == 1:
				self.seeprom.update_pin()
		elif pin == PIN_EEPROM_DO:
			self.seeprom.pin_state = state
		elif pin == PIN_AV0_I2C_CLOCK: pass
		elif pin == PIN_AV0_I2C_DATA: pass
		elif pin == PIN_AV1_I2C_CLOCK: pass
		elif pin == PIN_AV1_I2C_DATA: pass
		elif pin == PIN_BLUETOOTH_MODE: pass
		elif pin == PIN_WIFI_MODE: pass
		elif pin == PIN_31: pass
		else:
			print("GPIO PIN WRITE %i %i at %08X" %(pin, state, self.scheduler.pc()))
		
		
PIN_AV_RESET = 6
		
class GPIOGroup2:
	def __init__(self, scheduler, gpio):
		self.scheduler = scheduler
		self.gpio = gpio

	def read(self, espresso):
		return 0
		
	def write(self, pin, state, espresso):
		if pin == PIN_AV_RESET: pass
		else:
			print("GPIO2 PIN WRITE %i %i at %08X" %(pin, state, self.scheduler.pc()))

	
LT_GPIOE_OUT = 0
LT_GPIOE_DIR = 4
LT_GPIOE_IN = 8
LT_GPIOE_INTLVL = 0xC
LT_GPIOE_INTFLAG = 0x10
LT_GPIOE_INTMASK = 0x14
LT_GPIOE_INMIR = 0x18
LT_GPIO_ENABLE = 0x1C
LT_GPIO_OUT = 0x20
LT_GPIO_DIR = 0x24
LT_GPIO_IN = 0x28
LT_GPIO_INTLVL = 0x2C
LT_GPIO_INTFLAG = 0x30
LT_GPIO_INTMASK = 0x34
LT_GPIO_INMIR = 0x38
LT_GPIO_OWNER = 0x3C
	
class GPIOController:
	def __init__(self, scheduler):
		self.scheduler = scheduler
	
		self.gpioe_dir = 0
		self.gpioe_out = 0
		self.gpioe_intmask = 0
		self.gpioe_intflag = 0
		self.gpioe_intlvl = 0
	
		self.gpio_dir = 0
		self.gpio_enabled = 0xFFFFFFFF
		self.gpio_out = 0
		self.gpio_intmask = 0
		self.gpio_owner = 0
		self.gpio_intflag = 0
		self.gpio_intlvl = 0
		
		self.group = None
		
	def set_group(self, group): self.group = group

	def read(self, addr):
		if addr == LT_GPIOE_OUT: return self.gpioe_out
		elif addr == LT_GPIOE_DIR: return self.gpioe_dir
		elif addr == LT_GPIOE_INTLVL: return self.gpioe_intlvl
		elif addr == LT_GPIOE_INTFLAG: return self.gpioe_intflag
		elif addr == LT_GPIOE_INTMASK: return self.gpioe_intmask
	
		elif addr == LT_GPIO_ENABLE: return self.gpio_enabled
		elif addr == LT_GPIO_OUT: return self.gpio_out
		elif addr == LT_GPIO_DIR: return self.gpio_dir
		elif addr == LT_GPIO_IN: return self.group.read(False)
		elif addr == LT_GPIO_INTLVL: return self.gpio_intlvl
		elif addr == LT_GPIO_INTFLAG: return self.gpio_intflag
		elif addr == LT_GPIO_INTMASK: return self.gpio_intmask
		elif addr == LT_GPIO_OWNER: return self.gpio_owner
		print("GPIO READ 0x%X at %08X" %(addr, self.scheduler.pc()))
		return 0

	def write(self, addr, value):
		if addr == LT_GPIOE_OUT:
			for i in range(32):
				if self.gpio_owner & (1 << i):
					if value & (1 << i) != self.gpioe_out & (1 << i):
						self.group.write(i, (value >> i) & 1, True)
				self.gpioe_out = value
		elif addr == LT_GPIOE_DIR: self.gpioe_dir = value
		elif addr == LT_GPIOE_INTLVL: self.gpioe_intlvl = value
		elif addr == LT_GPIOE_INTFLAG: self.gpioe_intflag &= ~value
		elif addr == LT_GPIOE_INTMASK: self.gpioe_intmask = value
			
		elif addr == LT_GPIO_ENABLE: self.gpio_enabled = value
		elif addr == LT_GPIO_OUT:
			for i in range(32):
				if value & (1 << i) != self.gpio_out & (1 << i):
					self.group.write(i, (value >> i) & 1, False)
			self.gpio_out = value
		elif addr == LT_GPIO_DIR: self.gpio_dir = value
		elif addr == LT_GPIO_INTLVL: self.gpio_intlvl = value
		elif addr == LT_GPIO_INTFLAG: self.gpio_intflag &= ~value
		elif addr == LT_GPIO_INTMASK: self.gpio_intmask = value
		elif addr == LT_GPIO_OWNER: self.gpio_owner = value
		else:
			print("GPIO WRITE 0x%X %08X at %08X" %(addr, value, self.scheduler.pc()))
			
	def trigger_interrupt(self, type, espresso):
		if espresso: self.gpioe_intflag |= 1 << type
		else: self.gpio_intflag |= 1 << type
			
	def check_interrupts_ppc(self): return self.gpioe_intflag & self.gpioe_intmask
	def check_interrupts_arm(self): return self.gpio_intflag & self.gpio_intmask


I2C_CLOCK = 0
I2C_WRITE_DATA = 1
I2C_WRITE_CTRL = 2
I2C_READ_DATA = 3
I2C_INT_MASK = 4
I2C_INT_STATE = 5

LT_I2C_REGS = {
	0xD000570: I2C_CLOCK,
	0xD000574: I2C_WRITE_DATA,
	0xD000578: I2C_WRITE_CTRL,
	0xD00057C: I2C_READ_DATA,
	0xD000580: I2C_INT_MASK,
	0xD000584: I2C_INT_STATE
}

LT_I2C_REGS_PPC = {
	0xD000068: I2C_INT_MASK,
	0xD00006C: I2C_INT_STATE,
	0xD000250: I2C_CLOCK,
	0xD000254: I2C_WRITE_DATA,
	0xD000258: I2C_WRITE_CTRL,
	0xD00025C: I2C_READ_DATA
}

class I2CController:
	def __init__(self, scheduler, gpio2, espresso):
		self.scheduler = scheduler
		self.gpio2 = gpio2
		self.espresso = espresso
		
		self.readbuf = b""
		self.readoffs = 0
		self.readsize = False
		self.writebuf = b""
		self.writeval = 0
		self.offset = 0
		
		self.clock = 0
		self.int_mask = 0
		self.int_state = 0
		
		self.readint = 5 if espresso else 0
		self.writeint = 6 if espresso else 1
		
		self.av_int_mask = 0
		self.av_int_info = [0] * 6
		
	def read(self, addr):
		if addr == I2C_CLOCK: return self.clock
		elif addr == I2C_WRITE_DATA: return self.writeval
		elif addr == I2C_READ_DATA:
			if self.readsize:
				self.readsize = False
				return len(self.readbuf) << 16
			
			self.readoffs += 1
			return self.readbuf[self.readoffs]
		elif addr == I2C_WRITE_CTRL: return 0
		elif addr == I2C_INT_MASK: return self.int_mask
		elif addr == I2C_INT_STATE: return self.int_state
		
		raise RuntimeError("Read from I2C register %i" %addr)
		
	def write(self, addr, value):
		if addr == I2C_CLOCK: self.clock = value
		elif addr == I2C_WRITE_DATA: self.writeval = value
		elif addr == I2C_WRITE_CTRL:
			if value & 1:
				self.writebuf += bytes([self.writeval & 0xFF])
				if self.writeval & 0x100:
					self.handle_data(self.writebuf)
					self.writebuf = b""
		elif addr == I2C_INT_MASK: self.int_mask = value
		elif addr == I2C_INT_STATE: self.int_state &= ~value
		else:
			raise RuntimeError("Write to I2C register %i" %addr)
		
	def handle_data(self, data):
		slave = data[0] >> 1
		read = data[0] & 1
		
		if read:
			self.readbuf = self.read_data(slave, self.offset, len(data) - 1)
			self.readoffs = -1
			self.readsize = not self.espresso #This is weird
			self.trigger_interrupt(self.readint)
		else:
			if len(data) > 2:
				self.write_data(slave, data[1], data[2:])
			self.offset = data[1]
			self.trigger_interrupt(self.writeint)
		
	def read_data(self, slave, offset, length):
		if slave == 0x38 and offset == 0x90: return bytes([self.av_int_mask])
		elif slave == 0x38 and 0x91 <= offset <= 0x97: return bytes([self.av_int_info[offset - 0x91]])
	
		print("I2C READ 0x%X 0x%X %i" %(slave, offset, length))
		return bytes(length)
		
	def write_data(self, slave, offset, data):
		if slave == 0x3D and offset == 0x89:
			self.av_int_mask |= 0x10
			self.av_int_info[4] = 0
			self.gpio2.trigger_interrupt(4, True)
		else:
			print("I2C WRITE 0x%X 0x%X %s" %(slave, offset, data.hex()))
		
	def trigger_interrupt(self, type):
		self.int_state |= 1 << type
		
	def check_interrupts(self):
		return self.int_state & self.int_mask

			
class ASICBusController:
	def __init__(self, scheduler):
		self.scheduler = scheduler
	
		self.pll_data = [0xC, 0x800, 0x1C2, 7, 0, 0, 0x100, 0x40, 0xC800]
		self.usbpll_data = [0x20, 3, 0x1200, 0x3F, 0xC120]
		self.gfxpll_data = [0x1200, 0x20, 0xA, 0x800, 0xD, 0x800, 0x81C2, 0, 0x4002, 0]
		self.satapll_data = [0] * 9
	
	def set_offset(self, offset): self.offset = offset
	def get_data(self):
		if 0x3000010 <= self.offset < 0x3000022:
			return self.pll_data[(self.offset - 0x3000010) // 2]
		elif 0x4000024 <= self.offset < 0x400002E:
			return self.usbpll_data[(self.offset - 0x4000024) // 2]
		elif 0x878 <= self.offset < 0x88C:
			return self.gfxpll_data[(self.offset - 0x878) // 2]
		elif self.offset == 0x1000000:
			return 0x54
		elif 0x4000010 <= self.offset < 0x4000022:
			return self.satapll_data[(self.offset - 0x4000010) // 2]
		elif self.offset >> 24 == 0xC0: return 0

		print("ASIC BIF READ 0x%08X at %08X" %(self.offset, self.scheduler.pc()))
		return 0
		
	def write(self, value):
		if 0x3000010 <= self.offset < 0x3000022:
			self.pll_data[(self.offset - 0x3000010) // 2] = value
		elif 0x4000024 <= self.offset < 0x400002E:
			self.usbpll_data[(self.offset - 0x4000024) // 2] = value
		elif 0x4000010 <= self.offset < 0x4000022:
			self.satapll_data[(self.offset - 0x4000010) // 2] = value
		elif self.offset >> 24 == 0xC0: pass
		else:
			print("ASIC BIF WRITE 0x%08X %08X at %08X" %(self.offset, value, self.scheduler.pc()))
		

LT_IPC_PPCMSG = 0
LT_IPC_PPCCTRL = 4
LT_IPC_ARMMSG = 8
LT_IPC_ARMCTRL = 0xC
			
class IPCController:
	def __init__(self, scheduler):
		self.scheduler = scheduler
		
		self.ppcmsg = 0
		self.armmsg = 0
		self.x1 = self.x2 = 0
		self.y1 = self.y2 = 0
		self.ix1 = self.ix2 = 0
		self.iy1 = self.iy2 = 0

	def read(self, addr):
		if addr == LT_IPC_PPCMSG: return self.ppcmsg
		elif addr == LT_IPC_ARMMSG: return self.armmsg
		elif addr == LT_IPC_PPCCTRL:
			return self.x1 | (self.y2 << 1) | (self.y1 << 2) | \
				(self.x2 << 3) | (self.iy1 << 4) | (self.iy2 << 5)
		elif addr == LT_IPC_ARMCTRL:
			return self.y1 | (self.x2 << 1) | (self.x1 << 2) | \
				(self.y2 << 3) | (self.ix1 << 4) | (self.ix2 << 5)
		
	def write(self, addr, value):
		if addr == LT_IPC_PPCMSG: self.ppcmsg = value
		elif addr == LT_IPC_ARMMSG: self.armmsg = value

		elif addr == LT_IPC_PPCCTRL:
			if value & 1: self.x1 = 1
			if value & 2: self.y2 = 0
			if value & 4: self.y1 = 0
			if value & 8: self.x2 = 1
			self.iy1 = (value >> 4) & 1
			self.iy2 = (value >> 5) & 1

		elif addr == LT_IPC_ARMCTRL:
			if value & 1: self.y1 = 1
			if value & 2: self.x2 = 0
			if value & 4: self.x1 = 0
			if value & 8: self.y2 = 1
			self.ix1 = (value >> 4) & 1
			self.ix2 = (value >> 5) & 1
			
	def check_interrupts_ppc(self): return (self.y1 and self.iy1) or (self.y2 and self.iy2)
	def check_interrupts_arm(self): return (self.x1 and self.ix1) or (self.x2 and self.ix2)

			
LT_INTSR_AHBALL = 0
LT_INTSR_AHBLT = 4
LT_INTMR_AHBALL = 8
LT_INTMR_AHBLT = 0xC
LT_INTMR_AHBALL_2X = 0x10
LT_INTMR_AHBLT_2X = 0x14

class IRQController:
	def __init__(self):
		self.intsr_ahball = 0
		self.intsr_ahblt = 0
		self.intmr_ahball = 0
		self.intmr_ahblt = 0
		
	def read(self, addr):
		if addr == LT_INTSR_AHBALL: return self.intsr_ahball
		elif addr == LT_INTSR_AHBLT: return self.intsr_ahblt
		elif addr == LT_INTMR_AHBALL: return self.intmr_ahball
		elif addr == LT_INTMR_AHBLT: return self.intmr_ahblt
		
	def write(self, addr, value):
		if addr == LT_INTSR_AHBALL: self.intsr_ahball &= ~value
		elif addr == LT_INTSR_AHBLT: self.intsr_ahblt &= ~value
		elif addr == LT_INTMR_AHBALL: self.intmr_ahball = value
		elif addr == LT_INTMR_AHBLT: self.intmr_ahblt = value
		
	def trigger_irq_all(self, type): self.intsr_ahball |= (1 << type)
	def trigger_irq_lt(self, type): self.intsr_ahblt |= (1 << type)
	def is_triggered_all(self, type): return self.intsr_ahball & self.intmr_ahball & (1 << type)
	def is_triggered_lt(self, type): return self.intsr_ahblt & self.intmr_ahblt & (1 << type)
		
	def check_interrupts(self):
		self.update_interrupts()
		return self.intsr_ahball & self.intmr_ahball or self.intsr_ahblt & self.intmr_ahblt
	
	def update_interrupts(self): pass
	
class IRQControllerPPC(IRQController):
	def __init__(self, ipc, gpio, gpio2, i2c, index):
		super().__init__()
		self.ipc = ipc
		self.gpio = gpio
		self.gpio2 = gpio2
		self.i2c = i2c
		self.index = index
		
	def update_interrupts(self):
		if self.ipc.check_interrupts_ppc():
			self.trigger_irq_lt(30 - 2 * self.index)
		if self.gpio.check_interrupts_ppc() or self.gpio2.check_interrupts_ppc():
			self.trigger_irq_all(10)
		if self.i2c.check_interrupts():
			self.trigger_irq_lt(13)

class IRQControllerARM(IRQController):
	def __init__(self, ipc, gpio, gpio2, i2c):
		super().__init__()
		self.intmr_ahball_2x = 0
		self.intmr_ahblt_2x = 0
		
		self.ipc = ipc
		self.gpio = gpio
		self.gpio2 = gpio2
		self.i2c = i2c
		
	def read(self, addr):
		if addr == LT_INTMR_AHBALL_2X: return self.intmr_ahball_2x
		elif addr == LT_INTMR_AHBLT_2X: return self.intmr_ahblt_2x
		return super().read(addr)
		
	def write(self, addr, value):
		if addr == LT_INTMR_AHBALL_2X: self.intmr_ahball_2x = value
		elif addr == LT_INTMR_AHBLT_2X: self.intmr_ahblt_2x = value
		else: super().write(addr, value)
		
	def update_interrupts(self):
		for i in range(3):
			if self.ipc[i].check_interrupts_arm():
				self.trigger_irq_lt(31 - 2 * i)
		if self.gpio.check_interrupts_arm() or self.gpio2.check_interrupts_arm():
			self.trigger_irq_all(11)
		if self.i2c.check_interrupts():
			self.trigger_irq_lt(14)

class LatteController:
	def __init__(self, scheduler, debug=False):
		self.ipc = [IPCController(scheduler) for i in range(3)]
		self.gpio = GPIOController(scheduler)
		self.gpio.set_group(GPIOGroup1(scheduler, self.gpio))
		self.gpio2 = GPIOController(scheduler)
		self.gpio2.set_group(GPIOGroup2(scheduler, self.gpio2))
		self.i2c = I2CController(scheduler, self.gpio2, False)
		self.i2c_ppc = I2CController(scheduler, self.gpio2, True)

		self.irq_ppc = [IRQControllerPPC(self.ipc[i], self.gpio, self.gpio2, self.i2c_ppc, i) for i in range(3)]
		self.irq_arm = IRQControllerARM(self.ipc, self.gpio, self.gpio2, self.i2c)
		
		self.otp = OTPController(scheduler)
		self.asicbus = ASICBusController(scheduler)
	
		self.timer = 0
		self.alarm = 0
		self.ahb_wdg_config = 0
		self.error = 0
		self.error_mask = 0
		self.memirr = 0
		self.ahbprot = 0
		self.exi_ctrl = 0
		self.d800074 = 0
		self.d800180 = 0
		self.compat_memctrl_workaround = 0
		self.boot0 = 0
		self.clockinfo = 0
		self.resets_compat = 0
		self.clockgate_compat = 0
		self.d8001d0 = 0
		self.iopower = 0
		self.iostrength_ctrl0 = 0
		self.iostrength_ctrl1 = 0
		self.acrclk_strength_ctrl = 0
		self.otpcmd = 0
		self.otpdata = 0
		self.d800204 = 0
		self.d800500 = 0
		self.d800504 = 0
		self.debug = 0x80000000 * debug + 0x20000000
		self.compat_memctrl_state = 0
		self.iop2x = 0
		self.iostrength_ctrl2 = 0
		self.d8005cc = 0
		self.resets = 0
		self.resets_ahmn = 0
		self.syspll_cfg = 0
		self.d800628 = 0
		self.cfg_60xe = 0
		
		self.scheduler = scheduler

	def read(self, addr):
		if addr == LT_TIMER: return self.timer
		elif addr == LT_AHB_WDG_CONFIG: return self.ahb_wdg_config
		elif addr == LT_ERROR: return self.error
		elif addr == LT_ERROR_MASK: return self.error_mask
		elif addr == LT_MEMIRR: return self.memirr
		elif addr == LT_AHBPROT: return self.ahbprot
		elif addr == LT_EXICTRL: return self.exi_ctrl
		elif addr == LT_D800074: return self.d800074
		elif LT_GPIO_START <= addr < LT_GPIO_END: return self.gpio.read(addr - LT_GPIO_START)
		elif addr == LT_D800180: return self.d800180
		elif addr == LT_COMPAT_MEMCTRL_WORKAROUND: return self.compat_memctrl_workaround
		elif addr == LT_BOOT0: return self.boot0
		elif addr == LT_CLOCKINFO: return self.clockinfo
		elif addr == LT_RESETS_COMPAT: return self.resets_compat
		elif addr == LT_CLOCKGATE_COMPAT: return self.clockgate_compat
		elif addr == LT_D8001D0: return self.d8001d0
		elif addr == LT_IOPOWER: return self.iopower
		elif addr == LT_IOSTRENGTH_CTRL0: return self.iostrength_ctrl0
		elif addr == LT_IOSTRENGTH_CTRL1: return self.iostrength_ctrl1
		elif addr == LT_ACRCLK_STRENGTH_CTRL: return self.acrclk_strength_ctrl
		elif addr == LT_OTPCMD: return self.otpcmd
		elif addr == LT_OTPDATA: return self.otpdata
		elif addr == LT_D800204: return self.d800204
		elif addr == LT_ASICREV_ACR: return HARDWARE_VERSION_ACR
		elif LT_IPC_PPC0_START <= addr < LT_IPC_PPC0_END: return self.ipc[0].read(addr - LT_IPC_PPC0_START)
		elif LT_IPC_PPC1_START <= addr < LT_IPC_PPC1_END: return self.ipc[1].read(addr - LT_IPC_PPC1_START)
		elif LT_IPC_PPC2_START <= addr < LT_IPC_PPC2_END: return self.ipc[2].read(addr - LT_IPC_PPC2_START)
		elif LT_IRQ_PPC0_START <= addr < LT_IRQ_PPC0_END: return self.irq_ppc[0].read(addr - LT_IRQ_PPC0_START)
		elif LT_IRQ_PPC1_START <= addr < LT_IRQ_PPC1_END: return self.irq_ppc[1].read(addr - LT_IRQ_PPC1_START)
		elif LT_IRQ_PPC2_START <= addr < LT_IRQ_PPC2_END: return self.irq_ppc[2].read(addr - LT_IRQ_PPC2_START)
		elif LT_IRQ_ARM_START <= addr < LT_IRQ_ARM_END: return self.irq_arm.read(addr - LT_IRQ_ARM_START)
		elif addr == LT_D800500: return self.d800500
		elif addr == LT_D800504: return self.d800504
		elif LT_GPIO2_START <= addr < LT_GPIO2_END: return self.gpio2.read(addr - LT_GPIO2_START)
		elif addr == LT_ASICREV_CCR: return HARDWARE_VERSION_CCR
		elif addr == LT_DEBUG: return self.debug
		elif addr == LT_COMPAT_MEMCTRL_STATE: return self.compat_memctrl_state
		elif addr == LT_IOP2X: return self.iop2x
		elif addr == LT_IOSTRENGTH_CTRL2: return self.iostrength_ctrl2
		elif addr == LT_D8005CC: return self.d8005cc
		elif addr == LT_RESETS: return self.resets
		elif addr == LT_RESETS_AHMN: return self.resets_ahmn
		elif addr == LT_SYSPLL_CFG: return self.syspll_cfg
		elif addr == LT_ABIF_CPLTL_DATA: return self.asicbus.get_data()
		elif addr == LT_D800628: return self.d800628
		elif addr == LT_60XE_CFG: return self.cfg_60xe
		elif addr in LT_I2C_REGS: return self.i2c.read(LT_I2C_REGS[addr])
		elif addr in LT_I2C_REGS_PPC: return self.i2c_ppc.read(LT_I2C_REGS_PPC[addr])

		print("LATTE READ 0x%X at %08X" %(addr, self.scheduler.pc()))
		return 0
		
	def write(self, addr, value):
		if addr == LT_TIMER: self.timer = value
		elif addr == LT_ALARM: self.alarm = value
		elif addr == LT_AHB_WDG_CONFIG: self.ahb_wdg_config = value
		elif addr == LT_AHB_DMA_STATUS: pass
		elif addr == LT_AHB_CPU_STATUS: pass
		elif addr == LT_ERROR: self.error = value & self.error_mask
		elif addr == LT_ERROR_MASK: self.error_mask = value
		elif addr == LT_MEMIRR: self.memirr = value
		elif addr == LT_AHBPROT: self.ahbprot = value
		elif addr == LT_EXICTRL: self.exi_ctrl = value
		elif addr == LT_D800074: self.d800074 = value
		elif addr == LT_D800088: pass
		elif LT_GPIO_START <= addr < LT_GPIO_END: self.gpio.write(addr - LT_GPIO_START, value)
		elif addr == LT_D800180: self.d800180 = value
		elif addr == LT_COMPAT_MEMCTRL_WORKAROUND: self.compat_memctrl_workaround = value
		elif addr == LT_BOOT0: self.boot0 = value
		elif addr == LT_RESETS_COMPAT: self.resets_compat = value
		elif addr == LT_CLOCKGATE_COMPAT: self.clockgate_compat = value
		elif addr == LT_D8001D0: self.d8001d0 = value
		elif addr == LT_IOPOWER: self.iopower = value
		elif addr == LT_IOSTRENGTH_CTRL0: self.iostrength_ctrl0 = value
		elif addr == LT_IOSTRENGTH_CTRL1: self.iostrength_ctrl1 = value
		elif addr == LT_ACRCLK_STRENGTH_CTRL: self.acrclk_strength_ctrl = value
		elif addr == LT_OTPCMD:
			self.otpcmd = value
			if value & 0x80000000:
				self.otpdata = self.otp.read((value >> 8) & 7, value & 0x1F)
		elif addr == LT_D800204: self.d800204 = value
		elif LT_IPC_PPC0_START <= addr < LT_IPC_PPC0_END: self.ipc[0].write(addr - LT_IPC_PPC0_START, value)
		elif LT_IPC_PPC1_START <= addr < LT_IPC_PPC1_END: self.ipc[1].write(addr - LT_IPC_PPC1_START, value)
		elif LT_IPC_PPC2_START <= addr < LT_IPC_PPC2_END: self.ipc[2].write(addr - LT_IPC_PPC2_START, value)
		elif LT_IRQ_PPC0_START <= addr < LT_IRQ_PPC0_END: self.irq_ppc[0].write(addr - LT_IRQ_PPC0_START, value)
		elif LT_IRQ_PPC1_START <= addr < LT_IRQ_PPC1_END: self.irq_ppc[1].write(addr - LT_IRQ_PPC1_START, value)
		elif LT_IRQ_PPC2_START <= addr < LT_IRQ_PPC2_END: self.irq_ppc[2].write(addr - LT_IRQ_PPC2_START, value)
		elif LT_IRQ_ARM_START <= addr < LT_IRQ_ARM_END: self.irq_arm.write(addr - LT_IRQ_ARM_START, value)
		elif addr == LT_AHB2_DMA_STATUS: pass
		elif addr == LT_AHB2_CPU_STATUS: pass
		elif addr == LT_D800500: self.d800500 = value
		elif addr == LT_D800504: self.d800504 = value
		elif addr == LT_OTPPROT: pass
		elif LT_GPIO2_START <= addr < LT_GPIO2_END: self.gpio2.write(addr - LT_GPIO2_START, value)
		elif addr == LT_DEBUG: self.debug = value
		elif addr == LT_COMPAT_MEMCTRL_STATE: self.compat_memctrl_state = value
		elif addr == LT_IOP2X:
			self.iop2x = value | 4
			self.irq_arm.trigger_irq_lt(12)
		elif addr == LT_IOSTRENGTH_CTRL2: self.iostrength_ctrl2 = value
		elif addr == LT_D8005CC: self.d8005cc = value
		elif addr == LT_RESETS: self.resets = value
		elif addr == LT_RESETS_AHMN: self.resets_ahmn = value
		elif addr == LT_SYSPLL_CFG: self.syspll_cfg = value
		elif addr == LT_ABIF_CPLTL_OFFSET: self.asicbus.set_offset(value)
		elif addr == LT_ABIF_CPLTL_DATA: self.asicbus.write(value)
		elif addr == LT_D800628: self.d800628 = value
		elif addr == LT_60XE_CFG: self.cfg_60xe = value
		elif addr == LT_D800660: pass
		elif addr in LT_I2C_REGS: self.i2c.write(LT_I2C_REGS[addr], value)
		elif addr in LT_I2C_REGS_PPC: self.i2c_ppc.write(LT_I2C_REGS_PPC[addr], value)
		else:
			print("LATTE WRITE 0x%X %08X at %08X" %(addr, value, self.scheduler.pc()))
			
	def update_timer(self):
		start = self.timer
		end = self.timer + 400
		
		if end >= 0x100000000:
			if self.alarm > start or self.alarm <= end:
				self.irq_arm.trigger_irq_all(0)
			end &= 0xFFFFFFFF
		else:
			if start < self.alarm <= end:
				self.irq_arm.trigger_irq_all(0)
				
		self.timer = end


PI_CPU0_START = 0xC000078
PI_CPU0_END = 0xC000080
PI_CPU1_START = 0xC000080
PI_CPU1_END = 0xC000088
PI_CPU2_START = 0xC000088
PI_CPU2_END = 0xC000090

PI_INTSR = 0
PI_INTMR = 4

class PIController:
	def __init__(self, scheduler, irq, tcl, index):
		self.scheduler = scheduler
		self.irq = irq
		self.tcl = tcl
		self.index = index

		self.intsr = 0
		self.intmr = 0

	def read(self, addr):
		if addr == PI_INTSR: return self.intsr
		elif addr == PI_INTMR: return self.intmr
		print("PI(%i) READ 0x%X at %08X" %(self.index, addr, self.scheduler.pc()))
		return 0
		
	def write(self, addr, value):
		if addr == PI_INTSR: self.intsr &= ~value
		elif addr == PI_INTMR: self.intmr = value
		else:
			print("PI(%i) WRITE 0x%X %08X at %08X" %(self.index, addr, value, self.scheduler.pc()))

	def trigger_irq(self, type):
		self.intsr |= 1 << type
			
	def check_interrupts(self):
		self.update_interrupts()
		return self.intsr & self.intmr
		
	def update_interrupts(self):
		if self.irq.check_interrupts():
			self.trigger_irq(24)
		if self.tcl.check_interrupts():
			self.trigger_irq(23)

		#Both GPIO/I2C? (see tve.rpl)
		if self.irq.is_triggered_all(10):
			self.trigger_irq(10)
		if self.irq.is_triggered_lt(26 + self.index * 2):
			self.trigger_irq(26 + self.index * 2)
		if self.irq.is_triggered_lt(13):
			self.trigger_irq(13)
			
			
PAD_START = 0xC1E0000
PAD_END = 0xC200000
		
class PADController:
	def __init__(self, scheduler):
		self.scheduler = scheduler
		
	def read(self, addr):
		print("PAD READ 0x%X at %08X" %(addr, self.scheduler.pc()))
		return 0
		
	def write(self, addr, value):
		print("PAD WRITE 0x%X %08X at %08X" %(addr, value, self.scheduler.pc()))


#These are ignored for now:
#	TCL_CP_EOP_EVENT = 0xC208400
#	TCL_CP_EOP_ADDR = 0xC208408
#	TCL_CP_RINGBUF_STATS = 0xC208544
#	TCL_CP_STAT = 0xC208680
#	TCL_CP_BUSY_STAT = 0xC20867C
#	TCL_CP_ME_HEADER = 0xC208684
#	TCL_CP_PFP_HEADER = 0xC208688
#	TCL_CP_RB_RPTR = 0xC208700
#	TCL_CP_IB1 = 0xC208730
#	TCL_CP_IB1_SIZE = 0xC208738
#	TCL_CP_IB2 = 0xC20873C
#	TCL_CP_IB2_SIZE = 0xC208744

TCL_INTR_INFO_PTR = 0xC203E04
TCL_INTR_READ_POS = 0xC203E08
TCL_INTR_INFO_POS_PTR = 0xC203E14
TCL_RLC_MICROCODE_CTRL = 0xC203F2C
TCL_RLC_MICROCODE_DATA = 0xC203F30
TCL_DC_C206070 = 0xC206070
TCL_DC0_INT_MASK = 0xC2060DC
TCL_DC_C2064A0 = 0xC2064A0
TCL_DC1_INT_MASK = 0xC2068DC
TCL_CP_RESET = 0xC208020
TCL_FLUSH = 0xC208500
TCL_CP_RB_BASE = 0xC20C100
TCL_CP_READ_POS_PTR = 0xC20C10C
TCL_CP_WRITE_POS = 0xC20C114
TCL_CP_MICROCODE1_CTRL = 0xC20C150
TCL_CP_MICROCODE1_DATA = 0xC20C154
TCL_CP_MICROCODE2_CTRL = 0xC20C15C
TCL_CP_MICROCODE2_DATA = 0xC20C160
TCL_DRMDMA_READ_POS = 0xC20D008
TCL_DRMDMA_WRITE_POS = 0xC20D00C

TCL_START = 0xC200000
TCL_END = 0xC300000

class TCLController:
	def __init__(self, scheduler, physmem):
		self.scheduler = scheduler
		self.scheduler.add_alarm(50000000, self.trigger_vsync)
		self.physmem = physmem

		self.intr_info_ptr = 0
		self.intr_read_pos = 0
		self.intr_info_pos_ptr = 0
		self.rlc_microcode = [0] * 0x400
		self.rlc_microcode_pos = 0
		self.dc0_int_mask = 0
		self.dc1_int_mask = 0
		self.cp_ringbuf_base = 0
		self.cp_read_pos_ptr = 0
		self.cp_write_pos = 0
		self.cp_microcode1 = [0] * 0x350
		self.cp_microcode1_pos = 0
		self.cp_microcode2 = [0] * 0x550
		self.cp_microcode2_pos = 0
		self.drmdma_read_pos = 0
		self.drmdma_write_pos = 0
		
	def read(self, addr):
		if addr == TCL_RLC_MICROCODE_DATA:
			value = self.rlc_microcode[self.rlc_microcode_pos]
			self.rlc_microcode_pos += 1
			return value
		elif addr == TCL_DC_C206070: return 0x10000
		elif addr == TCL_DC_C2064A0: return 2
		elif addr == TCL_FLUSH:
			#Process command buffers here?
			self.physmem.write(self.cp_read_pos_ptr, struct.pack(">H", self.cp_write_pos))
			self.drmdma_read_pos = self.drmdma_write_pos
			return 0
		elif addr == TCL_CP_MICROCODE1_DATA:
			value = self.cp_microcode1[self.cp_microcode1_pos]
			self.cp_microcode1_pos += 1
			return value
		elif addr == TCL_CP_MICROCODE2_DATA:
			value = self.cp_microcode2[self.cp_microcode2_pos]
			self.cp_microcode2_pos += 1
			return value
		elif addr == TCL_DRMDMA_READ_POS: return self.drmdma_read_pos
		print("TCL READ 0x%X at %08X" %(addr, self.scheduler.pc()))
		return 0
		
	def write(self, addr, value):
		if addr == TCL_INTR_INFO_PTR: self.intr_info_ptr = value << 8
		elif addr == TCL_INTR_READ_POS: self.intr_read_pos = value
		elif addr == TCL_INTR_INFO_POS_PTR: self.intr_info_pos_ptr = value
		elif addr == TCL_RLC_MICROCODE_CTRL: self.rlc_microcode_pos = value
		elif addr == TCL_RLC_MICROCODE_DATA:
			self.rlc_microcode[self.rlc_microcode_pos] = value
			self.rlc_microcode_pos += 1
		elif addr == TCL_DC0_INT_MASK: self.dc0_int_mask = value
		elif addr == TCL_DC1_INT_MASK: self.dc1_int_mask = value
		elif addr == TCL_CP_RESET: pass
		elif addr == TCL_CP_RB_BASE: self.cp_ringbuf_base = value << 8
		elif addr == TCL_CP_READ_POS_PTR: self.cp_read_pos_ptr = value
		elif addr == TCL_CP_WRITE_POS: self.cp_write_pos = value
		elif addr == TCL_CP_MICROCODE1_CTRL: self.cp_microcode1_pos = value
		elif addr == TCL_CP_MICROCODE1_DATA:
			self.cp_microcode1[self.cp_microcode1_pos] = value
			self.cp_microcode1_pos += 1
		elif addr == TCL_CP_MICROCODE2_CTRL: self.cp_microcode2_pos = value
		elif addr == TCL_CP_MICROCODE2_DATA:
			self.cp_microcode2[self.cp_microcode2_pos] = value
			self.cp_microcode2_pos += 1
		elif addr == TCL_DRMDMA_WRITE_POS: self.drmdma_write_pos = value
		else:
			print("TCL WRITE 0x%X %08X at %08X" %(addr, value, self.scheduler.pc()))
			
	def get_intr_info_pos(self): return struct.unpack(">I", self.physmem.read(self.intr_info_pos_ptr, 4))[0]
	def set_intr_info_pos(self, pos): self.physmem.write(self.intr_info_pos_ptr, struct.pack(">I", pos))
			
	def trigger_interrupt(self, type, data1=0, data2=0, data3=0):
		pos = self.get_intr_info_pos()
		self.physmem.write(self.intr_info_ptr + pos * 4, struct.pack(">IIII", type, data1, data2, data3))
		self.set_intr_info_pos(pos + 4)

	def trigger_vsync(self):
		#avm.rpl seems to be signalling vsync on 'DVOCap' and 'TrigA' instead
		#of the real vsync interrupt, we're triggering 'TrigA' here
		if self.dc0_int_mask & 0x01000000: self.trigger_interrupt(2, 3)
		if self.dc1_int_mask & 0x01000000: self.trigger_interrupt(6, 3)

	def check_interrupts(self):
		return self.get_intr_info_pos() != self.intr_read_pos

		
TCL_D0000000 = 0xD0000000

class HardwareController:
	def __init__(self, scheduler, physmem):
		self.latte = LatteController(scheduler)

		self.pad = PADController(scheduler)
		self.tcl = TCLController(scheduler, physmem)
		self.pi = [PIController(scheduler, self.latte.irq_ppc[i], self.tcl, i) for i in range(3)]
		
		armirq = self.latte.irq_arm
		self.ahmn = AHMNController(scheduler)
		self.mem = MEMController(scheduler)
		self.exi = EXIController(scheduler, armirq)
		self.di2sata = DI2SATAController(scheduler)
		self.ehci0 = EHCIController(scheduler, 0)
		self.ehci1 = EHCIController(scheduler, 1)
		self.ehci2 = EHCIController(scheduler, 2)
		self.ohci00 = OHCIController(scheduler, armirq, physmem, 0)
		self.ohci01 = OHCIController(scheduler, armirq, physmem, 1)
		self.ohci1 = OHCIController(scheduler, armirq, physmem, 2)
		self.ohci2 = OHCIController(scheduler, armirq, physmem, 3)
		self.ahci = AHCIController(scheduler, armirq, physmem)
		self.sdio0 = SDIOController(scheduler, armirq, physmem, 0, SDIOController.TYPE_UNK)
		self.sdio1 = SDIOController(scheduler, armirq, physmem, 1, SDIOController.TYPE_SD)
		self.sdio2 = SDIOController(scheduler, armirq, physmem, 2, SDIOController.TYPE_UNK)
		self.wifi = SDIOController(scheduler, armirq, physmem, 3, SDIOController.TYPE_UNK)
		self.nand = NANDController(scheduler, armirq, physmem)
		self.aes = AESController(scheduler, armirq, physmem, 0)
		self.aess = AESController(scheduler, armirq, physmem, 1)
		self.sha = SHAController(scheduler, armirq, physmem, 0)
		self.shas = SHAController(scheduler, armirq, physmem, 1)
		
		self.scheduler = scheduler

	def read(self, addr, length):
		addr &= ~0x800000

		if length == 2:
			if MEM_START <= addr < MEM_END: value = self.mem.read(addr)
			elif PAD_START <= addr < PAD_END: value = self.pad.read(addr)
			else:
				print("HW READ(2) 0x%X at %08X" %(addr, self.scheduler.pc()))
				value = 0

			return struct.pack(">H", value)

		elif length == 4:
			if LATTE_START <= addr < LATTE_END: value = self.latte.read(addr)
			elif PI_CPU0_START <= addr < PI_CPU0_END: value = self.pi[0].read(addr - PI_CPU0_START)
			elif PI_CPU1_START <= addr < PI_CPU1_END: value = self.pi[1].read(addr - PI_CPU1_START)
			elif PI_CPU2_START <= addr < PI_CPU2_END: value = self.pi[2].read(addr - PI_CPU2_START)
			elif TCL_START <= addr < TCL_END: value = self.tcl.read(addr)
			elif AHMN_START <= addr < AHMN_END: value = self.ahmn.read(addr)
			elif EXI_START <= addr < EXI_END: value = self.exi.read(addr)
			elif DI2SATA_START <= addr < DI2SATA_END: value = self.di2sata.read(addr)
			elif EHCI0_START <= addr < EHCI0_END: value = self.ehci0.read(addr - EHCI0_START)
			elif EHCI1_START <= addr < EHCI1_END: value = self.ehci1.read(addr - EHCI1_START)
			elif EHCI2_START <= addr < EHCI2_END: value = self.ehci2.read(addr - EHCI2_START)
			elif OHCI00_START <= addr < OHCI00_END: value = self.ohci00.read(addr - OHCI00_START)
			elif OHCI01_START <= addr < OHCI01_END: value = self.ohci01.read(addr - OHCI01_START)
			elif OHCI1_START <= addr < OHCI1_END: value = self.ohci1.read(addr - OHCI1_START)
			elif OHCI2_START <= addr < OHCI2_END: value = self.ohci2.read(addr - OHCI2_START)
			elif AHCI_START <= addr < AHCI_END: value = self.ahci.read(addr)
			elif SDIO0_START <= addr < SDIO0_END: value = self.sdio0.read(addr - SDIO0_START)
			elif SDIO1_START <= addr < SDIO1_END: value = self.sdio1.read(addr - SDIO1_START)
			elif SDIO2_START <= addr < SDIO2_END: value = self.sdio2.read(addr - SDIO2_START)
			elif WIFI_START <= addr < WIFI_END: value = self.wifi.read(addr - WIFI_START)
			elif NAND_START <= addr < NAND_END: value = self.nand.read(addr)
			elif AES_START <= addr < AES_END: value = self.aes.read(addr - AES_START)
			elif AESS_START <= addr < AESS_END: value = self.aess.read(addr - AESS_START)
			elif SHA_START <= addr < SHA_END: value = self.sha.read(addr - SHA_START)
			elif SHAS_START <= addr < SHAS_END: value = self.shas.read(addr - SHAS_START)
			elif addr == TCL_D0000000: value = 0
			else:
				print("HW READ(4) 0x%X at %08X" %(addr, self.scheduler.pc()))
				value = 0

			return struct.pack(">I", value)
			
		else:
			raise RuntimeError("Invalid HW read length: %i" %length)
	
	def write(self, addr, data):
		addr &= ~0x800000
		
		if len(data) == 2:
			value = struct.unpack(">H", data)[0]
			if MEM_START <= addr < MEM_END: self.mem.write(addr, value)
			elif PAD_START <= addr < PAD_END: self.pad.write(addr, value)
			else:
				print("HW WRITE 0x%X %04X at %08X" %(addr, value, self.scheduler.pc()))
				
		elif len(data) == 4:
			value = struct.unpack(">I",  data)[0]
			if LATTE_START <= addr < LATTE_END: self.latte.write(addr, value)
			elif PI_CPU0_START <= addr < PI_CPU0_END: self.pi[0].write(addr - PI_CPU0_START, value)
			elif PI_CPU1_START <= addr < PI_CPU1_END: self.pi[1].write(addr - PI_CPU1_START, value)
			elif PI_CPU2_START <= addr < PI_CPU2_END: self.pi[2].write(addr - PI_CPU2_START, value)
			elif TCL_START <= addr < TCL_END: self.tcl.write(addr, value)
			elif AHMN_START <= addr < AHMN_END: self.ahmn.write(addr, value)
			elif MEM_START <= addr < MEM_END: self.mem.write(addr, value)
			elif EXI_START <= addr < EXI_END: self.exi.write(addr, value)
			elif DI2SATA_START <= addr < DI2SATA_END: self.di2sata.write(addr, value)
			elif EHCI0_START <= addr < EHCI0_END: self.ehci0.write(addr - EHCI0_START, value)
			elif EHCI1_START <= addr < EHCI1_END: self.ehci1.write(addr - EHCI1_START, value)
			elif EHCI2_START <= addr < EHCI2_END: self.ehci2.write(addr - EHCI2_START, value)
			elif OHCI00_START <= addr < OHCI00_END: self.ohci00.write(addr - OHCI00_START, value)
			elif OHCI01_START <= addr < OHCI01_END: self.ohci01.write(addr - OHCI01_START, value)
			elif OHCI1_START <= addr < OHCI1_END: self.ohci1.write(addr - OHCI1_START, value)
			elif OHCI2_START <= addr < OHCI2_END: self.ohci2.write(addr - OHCI2_START, value)
			elif AHCI_START <= addr < AHCI_END: self.ahci.write(addr, value)
			elif SDIO0_START <= addr < SDIO0_END: self.sdio0.write(addr - SDIO0_START, value)
			elif SDIO1_START <= addr < SDIO1_END: self.sdio1.write(addr - SDIO1_START, value)
			elif SDIO2_START <= addr < SDIO2_END: self.sdio2.write(addr - SDIO2_START, value)
			elif WIFI_START <= addr < WIFI_END: self.wifi.write(addr - WIFI_START, value)
			elif NAND_START <= addr < NAND_END: self.nand.write(addr, value)
			elif AES_START <= addr < AES_END: self.aes.write(addr - AES_START, value)
			elif AESS_START <= addr < AESS_END: self.aess.write(addr - AESS_START, value)
			elif SHA_START <= addr < SHA_END: self.sha.write(addr - SHA_START, value)
			elif SHAS_START <= addr < SHAS_END: self.shas.write(addr - SHAS_START, value)
			else:
				print("HW WRITE 0x%X %08X at %08X" %(addr, value, self.scheduler.pc()))
				
		else:
			raise RuntimeError("Invalid HW write length: %i" %len(data))
