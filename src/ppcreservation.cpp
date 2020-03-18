
#include "ppcreservation.h"

PPCReservation::PPCReservation() {
	reset();
}

void PPCReservation::reset() {
	owner = 0;
	addr = 0;
}

void PPCReservation::reserve(void *owner, uint32_t addr) {
	this->owner = owner;
	this->addr = addr;
}

bool PPCReservation::check(void *owner, uint32_t addr) {
	return this->owner == owner && this->addr == addr;
}

void PPCReservation::write(uint32_t addr) {
	if (this->addr == addr) {
		reset();
	}
}
