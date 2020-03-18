
#pragma once

#include <cstdint>

class SHA1 {
public:
	uint32_t h0;
	uint32_t h1;
	uint32_t h2;
	uint32_t h3;
	uint32_t h4;
	
	void update(void *data);
};
