
#include "common/logger.h"

void Logger::init(Level lev, OutputTextStream *stream) {
	level = lev;
	
	if (!stream) {
		stream = Sys::stderr;
	}
	logger = stream;
}

void Logger::prepare() {
	if (!logger) {
		logger = Sys::stderr;
	}
}

Logger::Level Logger::level = Logger::WARNING;
Ref<OutputTextStream> Logger::logger;
