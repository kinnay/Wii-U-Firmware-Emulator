
#include "class_ppcmmu.h"
#include "class_endian.h"
#include "errors.h"

PPCMMU::PPCMMU(PhysicalMemory *physmem)
	: physmem(physmem), sdr1(0), dbatu{}, dbatl{}, ibatu{}, ibatl{}, sr{}
{
	setRpnSize(20);
}

void PPCMMU::setRpnSize(int bits) {
	pageIndexShift = 32 - bits;
	pageIndexMask = (1 << (28 - pageIndexShift)) - 1;
	byteOffsetMask = (1 << pageIndexShift) - 1;
	apiShift = 22 - pageIndexShift;
	cache.invalidate();
}

bool PPCMMU::read32(uint32_t addr, uint32_t *value) {
	if (addr >= 0xFFE00000) {
		addr = addr - 0xFFE00000 + 0x08000000;
	}
	
	if (physmem->read(addr, value) < 0) return false;
	Endian::swap(value);
	return true;
}

bool PPCMMU::translate(uint32_t *addr, uint32_t length, Access type) {
	if (type == Instruction) {
		if (!instrTranslation) return true;
		if (cacheEnabled) {
			if (cache.translate(addr, type)) return true;
		}
		if (translateBAT(addr, ibatu, ibatl, type)) return true;
	}
	else {
		if (!dataTranslation) return true;
		if (cacheEnabled) {
			if (cache.translate(addr, type)) return true;
		}
		if (translateBAT(addr, dbatu, dbatl, type)) return true;
	}
	
	uint32_t segment = sr[*addr >> 28];
	if (segment >> 31) {
		RuntimeError("Direct-store segment: addr=0x%08x length=0x%08x", *addr, length);
		return false;
	}

	if (!(segment & 0x10000000) || type != Instruction) {
		uint32_t pageIndex = (*addr >> pageIndexShift) & pageIndexMask;
		uint32_t vsid = segment & 0xFFFFFF;

		bool key;
		if (supervisorMode) key = (segment >> 30) & 1; //Ks
		else key = (segment >> 29) & 1; //Kp
		
		uint32_t primaryHash = (vsid & 0x7FFFF) ^ pageIndex;
		if (searchPageTable(addr, vsid, pageIndex, primaryHash, false, key, type)) return true;
		if (searchPageTable(addr, vsid, pageIndex, ~primaryHash, true, key, type)) return true;
	}
	
	ValueError("Illegal memory access: addr=0x%08x length=0x%08x", *addr, length);
	return false;
}

bool PPCMMU::translateBAT(uint32_t *addr, uint32_t *batu, uint32_t *batl, Access type) {
	bool write = type == DataWrite;
	for (int i = 0; i < 8; i++) {
		//Check read/write protection
		int pp = batl[i] & 3;
		if (!pp || ((pp & 1) && write)) continue;
		
		//Check user/supervisor mode
		int vp = batu[i] & 1;
		int vs = batu[i] & 2;
		if (!((vp && !supervisorMode) || (vs && supervisorMode))) continue;
		
		//Check block index and size
		uint32_t addrMask = ~((batu[i] >> 2) & 0x7FF);
		uint32_t effectiveBlock = batu[i] >> 17;
		uint32_t addrBlock = *addr >> 17;
		if ((effectiveBlock & addrMask) != (addrBlock & addrMask)) continue;
		
		//Translate address
		uint32_t brpn = batl[i] >> 17;
		addrBlock = (addrBlock & ~addrMask) | (brpn & addrMask);
		cache.update(type, *addr, addrBlock << 17, 0x1FFFF);
		*addr = (*addr & 0x1FFFF) | (addrBlock << 17);
		return true;
	}
	return false;
}

bool PPCMMU::searchPageTable(
	uint32_t *addr, uint32_t vsid, uint32_t pageIndex,
	uint32_t hash, bool secondary, int key, Access type
) {
	bool write = type == DataWrite;
	uint32_t pageTable = sdr1 & 0xFFFF0000;
	uint32_t pageMask = sdr1 & 0x1FF;
	uint32_t maskedHash = hash & ((pageMask << 10) | 0x3FF);
	uint32_t api = pageIndex >> apiShift;
	
	uint32_t pteAddr = pageTable | (maskedHash << 6);
	for (int i = 0; i < 8; i++, pteAddr += 8) {
		//Read PTE
		uint32_t pteHi, pteLo;
		if (!read32(pteAddr, &pteHi)) return false;
		if (!read32(pteAddr+4, &pteLo)) return false;
		
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
		cache.update(type, *addr, pteLo & 0xFFFFF000, byteOffsetMask);
		*addr = (pteLo & 0xFFFFF000) | (*addr & byteOffsetMask);
		return true;
	}
	return false;
}
