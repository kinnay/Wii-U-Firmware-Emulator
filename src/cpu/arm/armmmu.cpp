
#include "armmmu.h"
#include "common/logger.h"


ARMMMU::ARMMMU(PhysicalMemory *physmem, ARMCore *core) {
	this->physmem = physmem;
	this->core = core;
	reset();
}

void ARMMMU::reset() {
	cache.reset();
}

bool ARMMMU::checkPermissions(MemoryAccess type, bool supervisor, int ap) {
	if (ap == 0) return false;
	else {
		if (supervisor) return true;
		
		if (ap == 1) return false;
		if (ap == 2) return type != MemoryAccess::DataWrite;
		return true;
	}
}

void ARMMMU::signalFault(MemoryAccess type, uint32_t addr, int domain, int status) {
	core->far = addr;
	if (type == MemoryAccess::Instruction) {
		core->ifsr = status;
	}
	else {
		core->dfsr = status | (domain << 4);
		if (type == MemoryAccess::DataWrite) {
			core->dfsr |= 0x800;
		}
	}
}

bool ARMMMU::translate(uint32_t *addr, MemoryAccess type, bool supervisor) {
	if (!(core->control & 1)) return true;
	if (cache.translate(addr, type, supervisor)) {
		return true;
	}
	
	return translateFromTable(addr, type, supervisor);
}

bool ARMMMU::translateFromTable(uint32_t *addr, MemoryAccess type, bool supervisor) {
	uint32_t firstLevelOffs = (*addr >> 20) * 4;
	uint32_t firstLevelAddr = core->ttbr + firstLevelOffs;
	uint32_t firstLevelDesc = physmem->read<uint32_t>(firstLevelAddr);
	
	int domain = (firstLevelDesc >> 5) & 0xF;
	int firstLevelType = firstLevelDesc & 3;
	if (firstLevelType == 0) {
		signalFault(type, *addr, domain, 5);
		return false;
	}
	else if (firstLevelType == 1) {
		uint32_t secondLevelBase = firstLevelDesc & ~0x3FF;
		uint32_t secondLevelOffs = ((*addr >> 12) & 0xFF) * 4;
		uint32_t secondLevelAddr = secondLevelBase + secondLevelOffs;
		uint32_t secondLevelDesc = physmem->read<uint32_t>(secondLevelAddr);
		
		int secondLevelType = secondLevelDesc & 3;
		if (secondLevelType == 0) {
			signalFault(type, *addr, domain, 7);
			return false;
		}
		else if (secondLevelType == 2) {
			int subpage = (*addr & 0xFFF) / 0x400;
			int ap = (secondLevelDesc >> (4 + subpage * 2)) & 3;
			if (!checkPermissions(type, supervisor, ap)) {
				signalFault(type, *addr, domain, 15);
				return false;
			}
			uint32_t pageBase = secondLevelDesc & ~0xFFF;
			cache.update(type, supervisor, *addr, pageBase, 0xFFF);
			*addr = pageBase + (*addr & 0xFFF);
		}
		else {
			Logger::warning("Unsupported second-level descriptor type: %i", secondLevelType);
			return false;
		}
	}
	else if (firstLevelType == 2) {
		int ap = (firstLevelDesc >> 10) & 3;
		if (!checkPermissions(type, supervisor, ap)) {
			signalFault(type, *addr, domain, 13);
			return false;
		}
		
		uint32_t sectionBase = firstLevelDesc & ~0xFFFFF;
		cache.update(type, supervisor, *addr, sectionBase, 0xFFFFF);
		*addr = sectionBase + (*addr & 0xFFFFF);
	}
	else {
		Logger::warning("Unsupported first-level descriptor type: %i", firstLevelType);
		return false;
	}
	return true;
}
