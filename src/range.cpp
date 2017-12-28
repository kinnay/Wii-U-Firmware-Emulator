
#include "range.h"
#include <cstdint>

Range::Range(uint32_t start, uint32_t length) : start(start), length(length) {}

bool Range::contains(uint32_t value) {
	return start <= value && value - start < length;
}

bool Range::collides(uint32_t start, uint32_t length) {
	return start - this->start < this->length || this->start - start < length;
}

bool Range::collides(Range *range) {
	return collides(range->start, range->length);
}

bool Range::contains(uint32_t start, uint32_t length) {
	return start >= this->start && length <= this->length &&
		start - this->start <= this->length - length;
}

bool Range::contains(Range *range) {
	return contains(range->start, range->length);
}
