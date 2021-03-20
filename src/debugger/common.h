
#pragma once

#include <cstdint>

static uint64_t divide(uint64_t a, uint64_t b) {
	return b ? a / b : 0;
}

static int percentage(uint64_t value, uint64_t total) {
	return divide(value * 100, total);
}
