
#pragma once

#include <cstdint>

class PPCLockMgr {
	public:
	PPCLockMgr();
	void reset();
	void reserve(void *owner, uint32_t address);
	bool isReserved(void *owner, uint32_t address);
	
	private:
	void *owner;
	uint32_t address;
};
