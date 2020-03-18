
#pragma once

#include <cstdint>


class PPCReservation {
public:
	PPCReservation();
	
	void reset();
	void reserve(void *owner, uint32_t addr);
	bool check(void *owner, uint32_t addr);
	void write(uint32_t addr);
	
private:
	void *owner;
	uint32_t addr;
};
