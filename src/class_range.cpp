
#include "class_range.h"
#include <cstdint>

Range::Range(uint32_t start, uint32_t end) : start(start), end(end) {}

bool Range::contains(uint32_t value) {
	return start <= value && value <= end;
}

bool Range::collides(uint32_t start, uint32_t end) {
	return end >= this->start && start <= this->end;
}

bool Range::collides(Range *range) {
	return collides(range->start, range->end);
}

bool Range::contains(uint32_t start, uint32_t end) {
	return start >= this->start && end <= this->end;
}

bool Range::contains(Range *range) {
	return contains(range->start, range->end);
}
