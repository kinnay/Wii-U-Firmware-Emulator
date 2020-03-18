
#include "common/random.h"
#include <ctime>

Random::Random() {
	init();
}

void Random::init() {
	init(time(nullptr));
}

void Random::init(uint32_t seed) {
	uint32_t mult = 0x6C078965;
	
	uint32_t temp = seed;
	for (int i = 0; i < 4; i++) {
		temp ^= temp >> 30;
		temp = temp * mult + i + 1;
		state[i] = temp;
	}
}

uint32_t Random::u32() {
	uint32_t temp = state[0];
	temp ^= temp << 11;
	temp ^= temp >> 8;
	temp ^= state[3];
	temp ^= state[3] >> 19;
	state[0] = state[1];
	state[1] = state[2];
	state[2] = state[3];
	state[3] = temp;
	return temp;
}

uint32_t Random::u32(uint32_t max) {
	uint64_t value = u32();
	return (value * max) >> 32;
}

uint32_t Random::u32(uint32_t min, uint32_t max) {
	return min + u32(max - min);
}

float Random::f32() {
	return (float)u32() / 0x100000000;
}

float Random::f32(float max) {
	return f32() * max;
}

float Random::f32(float min, float max) {
	return min + f32(max - min);
}

int Random::get(int min, int max) {
	return u32(min, max);
}

Ref<Random> Random::inst = new Random();
