
#include "ppcmmu.h"


PPCMMU::PPCMMU(PhysicalMemory *physmem, PPCCore *core) {
	this->physmem = physmem;
	this->core = core;
}

void PPCMMU::reset() {
	cache.reset();
}

bool PPCMMU::translate(uint32_t *addr, MemoryAccess type, bool supervisor) {
	if (!translateVirtual(addr, type, supervisor)) {
		return false;
	}
	translatePhysical(addr);
	return true;
}

void PPCMMU::translatePhysical(uint32_t *addr) {
	if (*addr >= 0xFFE00000) {
		*addr = *addr - 0xFFE00000 + 0x08000000;
	}
}

bool PPCMMU::translateVirtual(uint32_t *addr, MemoryAccess type, bool supervisor) {
	if (type == MemoryAccess::Instruction) {
		if (!(core->msr & 0x20)) return true;
		if (cache.translate(addr, type, supervisor)) return true;
		if (translateBAT(addr, PPCCore::IBAT0U, type, supervisor)) return true;
	}
	else {
		if (!(core->msr & 0x10)) return true;
		if (cache.translate(addr, type, supervisor)) return true;
		if (translateBAT(addr, PPCCore::DBAT0U, type, supervisor)) return true;
	}
	
	uint32_t segment = core->sr[*addr >> 28];
	if (segment >> 31) {
		return false;
	}
	
	if (!(segment & 0x10000000) || type != MemoryAccess::Instruction) {
		uint32_t pageIndex = (*addr >> 17) & 0x7FF;
		uint32_t vsid = segment & 0xFFFFFF;
		
		bool key = supervisor ? (segment >> 30) & 1 : (segment >> 29) & 1;
		
		uint32_t primaryHash = (vsid & 0x7FFFF) ^ pageIndex;
		if (searchPageTable(addr, vsid, pageIndex, primaryHash, false, key, type, supervisor)) return true;
		if (searchPageTable(addr, vsid, pageIndex, ~primaryHash, true, key, type, supervisor)) return true;
	}
	
	return false;
}

bool PPCMMU::translateBAT(uint32_t *addr, int bat, MemoryAccess type, bool supervisor) {
	bool write = type == MemoryAccess::DataWrite;
	for (int i = 0; i < 8; i++) {
		uint32_t batu = core->sprs[bat + i * 2 + i / 4 * 8];
		uint32_t batl = core->sprs[bat + i * 2 + i / 4 * 8 + 1];
		
		int pp = batl & 3;
		if (!pp || ((pp & 1) && write)) continue;
		
		bool vp = batu & 1;
		bool vs = batu & 2;
		if (!((vp && !supervisor) || (vs && supervisor))) continue;
		
		uint32_t addrMask = ~((batu >> 2) & 0x7FF);
		uint32_t effectiveBlock = batu >> 17;
		uint32_t addrBlock = *addr >> 17;
		if ((effectiveBlock & addrMask) != (addrBlock & addrMask)) continue;
		
		uint32_t brpn = batl >> 17;
		addrBlock = (addrBlock & ~addrMask) | (brpn & addrMask);
		cache.update(type, supervisor, *addr, addrBlock << 17, 0x1FFFF);
		*addr = (*addr & 0x1FFFF) | (addrBlock << 17);
		return true;
	}
	return false;
}

bool PPCMMU::searchPageTable(
	uint32_t *addr, uint32_t vsid, uint32_t pageIndex,
	uint32_t hash, bool secondary, int key, MemoryAccess type,
	bool supervisor
) {
	bool write = type == MemoryAccess::DataWrite;
	uint32_t sdr1 = core->sprs[PPCCore::SDR1];
	uint32_t pageTable = sdr1 & 0xFFFF0000;
	uint32_t pageMask = sdr1 & 0x1FF;
	uint32_t maskedHash = hash & ((pageMask << 10) | 0x3FF);
	uint32_t api = pageIndex >> 5;
	
	uint32_t pteAddr = pageTable | (maskedHash << 6);
	translatePhysical(&pteAddr);
	
	for (int i = 0; i < 8; i++, pteAddr += 8) {
		//Read PTE
		uint32_t pteHi = physmem->read<uint32_t>(pteAddr);
		uint32_t pteLo = physmem->read<uint32_t>(pteAddr + 4);
		
		//Check validity
		if (!pteHi >> 31) continue;
		if (((pteHi >> 6) & 1) != secondary) continue;
		if (((pteHi >> 7) & 0xFFFFFF) != vsid) continue;
		if ((pteHi & 0x3F) != api) continue;
		
		//Check protection
		int pp = pteLo & 3;
		if (key && !pp) continue;
		if (write && (pp == 3 || (key && pp == 1))) continue;
		
		//Translate address
		cache.update(type, supervisor, *addr, pteLo & 0xFFFFF000, 0x1FFFF);
		*addr = (pteLo & 0xFFFFF000) | (*addr & 0x1FFFF);
		return true;
	}
	return false;
}