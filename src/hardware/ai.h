
#pragma once

#include <cstdint>

class AIController {
public:
	enum Register {
		AI_CONTROL = 0xD006C00,
		AI_VOLUME = 0xD006C04,
		AI_AISCNT = 0xD006C08,
		AI_AIIT = 0xD006C0C
	};
	
	void reset();
	void update();
	
	uint32_t read(uint32_t addr);
	void write(uint32_t addr, uint32_t value);
	
private:
	int timer;
	
	uint32_t control;
	uint32_t volume;
	uint32_t samples;
};
