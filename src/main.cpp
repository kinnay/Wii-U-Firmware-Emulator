
#include "emulator.h"
#include "common/logger.h"
#include "common/history.h"

int main(int argc, const char *argv[]) {
	Logger::init(Logger::DEBUG);
	History::setup();

	Emulator *emulator = new Emulator();
	emulator->run();
	delete emulator;

	return 0;
}
