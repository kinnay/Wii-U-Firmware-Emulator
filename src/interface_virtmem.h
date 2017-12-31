
#pragma once

#include "class_mmucache.h"
#include <cstdint>

class IVirtualMemory {
	public:
	enum Access {
		Instruction,
		DataRead,
		DataWrite
	};
	
	IVirtualMemory();
	
	virtual bool translate(uint32_t *addr, uint32_t length, Access type) = 0;
	void setCacheEnabled(bool enabled);
	void invalidateCache();
	
	protected:
	MMUCache cache;
	bool cacheEnabled;
};
