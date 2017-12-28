
#pragma once

#include <cstdint>

class Range {
	public:
	uint32_t start;
	uint32_t length;
	
	Range(uint32_t start, uint32_t length);
	bool contains(uint32_t value);
	bool collides(uint32_t start, uint32_t length);
	bool collides(Range *range);
	bool contains(uint32_t start, uint32_t length);
	bool contains(Range *range);
};
