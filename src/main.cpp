
#include "emulator.h"
#include "common/logger.h"

int main(int argc, const char *argv[]) {
	Logger::init(Logger::DEBUG);
	
	Emulator *emulator = new Emulator();
	emulator->run();
	delete emulator;

	return 0;
}
