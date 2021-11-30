
#include "emulator.h"
#include "history.h"
#include "common/logger.h"

int main(int argc, const char *argv[]) {
	Logger::init(Logger::DEBUG);
	History::init();

	Emulator *emulator = new Emulator();
	emulator->run();
	delete emulator;

	return 0;
}
