
#pragma once

#include "common/sys.h"
#include "common/outputtextstream.h"
#include "common/stringutils.h"
#include "common/refcountedobj.h"

const char symbols[] = {'-', 'E', 'W', 'I', 'D', 'T'};

class Logger {
public:
	enum Level {
		NONE,
		ERROR,
		WARNING,
		INFO,
		DEBUG,
		TRACE
	};
	
	static void init(Level level = WARNING, OutputTextStream *stream = nullptr);
	
	template <class... Args>
	static void error(const char *msg, Args... args) {
		log(ERROR, msg, args...);		
	}
	
	template <class... Args>
	static void warning(const char *msg, Args... args) {
		log(WARNING, msg, args...);		
	}
	
	template <class... Args>
	static void info(const char *msg, Args... args) {
		log(INFO, msg, args...);		
	}
	
	template <class... Args>
	static void debug(const char *msg, Args... args) {
		log(DEBUG, msg, args...);		
	}
	
	template <class... Args>
	static void trace(const char *msg, Args... args) {
		log(TRACE, msg, args...);		
	}
	
private:
	static void prepare();
	
	template <class... Args>
	static void log(Level lev, const char *msg, Args... args) {
		prepare();
		if (lev <= level) {
			std::string str = StringUtils::format(msg, args...);
			logger->write("[%c] %s\n", symbols[lev], str);
		}
	}
	
	static Level level;
	static Ref<OutputTextStream> logger;
};

