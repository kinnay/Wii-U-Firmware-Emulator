
#pragma once

#include <cstdint>

class Range {
	public:
	uint32_t start;
	uint32_t end;
	
	Range(uint32_t start, uint32_t end);
	bool contains(uint32_t value);
	bool collides(uint32_t start, uint32_t end);
	bool collides(Range *range);
	bool contains(uint32_t start, uint32_t end);
	bool contains(Range *range);
};
