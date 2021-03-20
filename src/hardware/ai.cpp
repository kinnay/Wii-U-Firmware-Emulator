
#include "hardware/ai.h"

#include "common/logger.h"

void AIController::reset() {
	timer = 0;
	
	control = 0;
	volume = 0;
	samples = 0;
}

void AIController::update() {
	int limit = control & 0x40 ? 60 : 40;
	
	timer++;
	if (timer == limit) {
		samples++;
		timer = 0;
	}
}

uint32_t AIController::read(uint32_t addr) {
	switch (addr) {
		case AI_CONTROL: return control;
		case AI_VOLUME: return volume;
		case AI_AISCNT: return samples;
	}
	Logger::warning("Unknown ai read: 0x%X", addr);
	return 0;
}

void AIController::write(uint32_t addr, uint32_t value) {
	if (addr == AI_CONTROL) {
		control = value;
		if (value & 0x20) {
			samples = 0;
		}
	}
	else if (addr == AI_VOLUME) volume = value;
	else {
		Logger::warning("Unknown ai write: 0x%X (0x%08X)", addr, value);
	}
}
