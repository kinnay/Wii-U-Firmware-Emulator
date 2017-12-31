
#include "class_armmmu.h"
#include "class_endian.h"
#include "errors.h"
#include <cstdint>

ARMMMU::ARMMMU(IPhysicalMemory *physmem, bool bigEndian) : physmem(physmem), translationTableBase(0), enabled(false), cacheEnabled(false) {
	swapEndian = bigEndian != (Endian::getSystemEndian() == Endian::Big);
}

void ARMMMU::setTranslationTableBase(uint32_t base) {
	translationTableBase = base;
	cache.invalidate();
}

void ARMMMU::setEnabled(bool enabled) { this->enabled = enabled; }
void ARMMMU::setCacheEnabled(bool enabled) { cacheEnabled = enabled; }
void ARMMMU::invalidateCache() { cache.invalidate(); }

bool ARMMMU::read32(uint32_t addr, uint32_t *value) {
	if (physmem->read(addr, value) < 0) return false;
	if (swapEndian) Endian::swap(value);
	return true;
}

bool ARMMMU::translate(uint32_t *addr, uint32_t length, Access type) {
	if (!enabled) return true;
	if (cacheEnabled) {
		if (cache.translate(addr, length, type)) return true;
	}

	int translationTableOffs = (*addr >> 20) * 4;
	
	uint32_t firstLevelDesc;
	if (!read32(translationTableBase + translationTableOffs, &firstLevelDesc))
		return false;
	
	int firstLevelType = firstLevelDesc & 3;
	if (firstLevelType == 0) { //Fault
		return false;
	}
	else if (firstLevelType == 1) { //Coarse page table
		uint32_t coarseTableBase = firstLevelDesc & ~0x3FF;
		int coarseTableOffs = ((*addr >> 12) & 0xFF) * 4;
		
		uint32_t secondLevelDesc;
		if (!read32(coarseTableBase + coarseTableOffs, &secondLevelDesc))
			return false;
		
		int secondLevelType = secondLevelDesc & 3;
		if (secondLevelType == 0) { //Fault
			return false;
		}
		else if (secondLevelType == 2) { //Small page
			uint32_t pageBase = secondLevelDesc & ~0xFFF;
			cache.update(type, *addr, pageBase, 0xFFF);
			*addr = pageBase + (*addr & 0xFFF);
			return true;
		}
		else {
			RuntimeError("Unsupported second-level descriptor type: %i", secondLevelType);
			return false;
		}
	}
	else if (firstLevelType == 2) { //Section
		uint32_t sectionBase = firstLevelDesc & ~0xFFFFF;
		cache.update(type, *addr, sectionBase, 0xFFFFF);
		*addr = sectionBase + (*addr & 0xFFFFF);
		return true;
	}
	else {
		RuntimeError("Unsupported first-level descriptor type: %i", firstLevelType);
		return false;
	}
}
