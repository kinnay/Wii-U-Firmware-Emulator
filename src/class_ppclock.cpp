
#include "class_ppclock.h"

PPCLockMgr::PPCLockMgr() {
	reset();
}

void PPCLockMgr::reset() {
	owner = 0;
	address = 0;
}

void PPCLockMgr::reserve(void *owner, uint32_t address) {
	this->owner = owner;
	this->address = address;
}

bool PPCLockMgr::isReserved(void *owner, uint32_t address) {
	return this->owner == owner && this->address == address;
}
