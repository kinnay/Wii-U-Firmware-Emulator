
#pragma once

#include "common/stringutils.h"
#include <stdexcept>

template <class T, class... Args>
void generic_error(const char *msg, Args... args) {
	throw T(StringUtils::format(msg, args...));
}

template <class... Args>
void runtime_error(const char *msg, Args... args) {
	generic_error<std::runtime_error>(msg, args...);
}

template <class... Args>
void logic_error(const char *msg, Args... args) {
	generic_error<std::logic_error>(msg, args...);
}

template <class... Args>
void invalid_argument(const char *msg, Args... args) {
	generic_error<std::invalid_argument>(msg, args...);
}
