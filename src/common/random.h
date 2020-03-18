
#pragma once

#include "common/refcountedobj.h"
#include <cstdint>

class Random : public RefCountedObj {
public:
	Random();
	
	void init();
	void init(uint32_t seed);
	
	uint32_t u32();
	uint32_t u32(uint32_t max);
	uint32_t u32(uint32_t min, uint32_t max);
	
	float f32();
	float f32(float max);
	float f32(float min, float max);
	
	int get(int min, int max);
	
	static Ref<Random> inst;
	
private:
	uint32_t state[4];
};
