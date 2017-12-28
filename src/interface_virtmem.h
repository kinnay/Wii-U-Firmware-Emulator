
#pragma once

#include <cstdint>

class IVirtualMemory {
	public:
	enum Access {
		Instruction,
		DataRead,
		DataWrite
	};
	
	virtual bool translate(uint32_t *addr, uint32_t length, Access type) = 0;
};
