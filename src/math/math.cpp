
#include "math/math.h"

float radians(float degrees) {
	return degrees * PI / 180;
}

float degrees(float radians) {
	return radians * 180 / PI;
}

float moveValueTo(float value, float target, float step) {
	float diff = fabs(target - value);
	if (diff < step) {
		return target;
	}
	if (target < value) {
		return value - step;
	}
	return value + step;
}

float moveAngleTo(float value, float target, float step) {
	float diff = fabs(target - value);
	while (diff > PI) {
		if (target < value) {
			target += PI * 2;
		}
		else {
			target -= PI * 2;
		}
		diff = fabs(target - value);
	}
	return fmod(moveValueTo(target, value, step), PI * 2);
}

size_t align(size_t value, size_t alignment) {
	return value + (alignment - value % alignment) % alignment;
}
