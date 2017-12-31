
#pragma once

#include "interface_virtmem.h"
#include "interface_physmem.h"
#include <cstdint>

class ARMMMU : public IVirtualMemory {
	public:
	ARMMMU(IPhysicalMemory *phys, bool bigEndian);
	bool translate(uint32_t *addr, uint32_t length, Access type);
	
	void setTranslationTableBase(uint32_t base);
	void setEnabled(bool enabled);
	
	private:
	IPhysicalMemory *physmem;
	uint32_t translationTableBase;
	bool enabled;
	
	bool swapEndian;
	bool read32(uint32_t addr, uint32_t *value);
};
