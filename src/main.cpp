
#include "emulator.h"
#include "history.h"
#include "common/logger.h"

int main(int argc, const char *argv[]) {
	Logger::init(Logger::DEBUG);
	History::init();

	bool boot0 = false;
	if (argc > 1 && std::strcmp(argv[1], "--boot0") == 0) {
		boot0 = true;
	}

	Emulator *emulator = new Emulator(boot0);
	emulator->run();
	delete emulator;

	return 0;
}
