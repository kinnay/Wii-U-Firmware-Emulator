
#pragma once

#include "interface_virtmem.h"
#include "class_physmem.h"
#include <cstdint>

class ARMMMU : public IVirtualMemory {
public:
	ARMMMU(PhysicalMemory *phys);
	bool translate(uint32_t *addr, uint32_t length, Access type);
	
	void setTranslationTableBase(uint32_t base);
	
private:
	PhysicalMemory *physmem;
	uint32_t translationTableBase;
	
	bool read32(uint32_t addr, uint32_t *value);
};
