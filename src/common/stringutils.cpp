
#include "common/stringutils.h"
#include "common/exceptions.h"
#include <cctype>


StringFormatter::StringFormatter(std::string msg) {
	this->msg = msg;
	pos = 0;
}

std::string StringFormatter::format() {
	std::string s;
	
	char c = read();
	while (c) {
		if (c == '%') {
			char next = read();
			if (next == '%') {
				s += '%';
			}
			else {
				runtime_error("Not enough arguments for string formatting");
			}
		}
		else {
			s += c;
		}
		c = read();
	}
	return s;
}

char StringFormatter::read() {
	return pos >= msg.size() ? '\0' : msg[pos++];
}


bool StringUtils::is_printable(char c) {
	return c >= ' ' && c <= '~';
}

bool StringUtils::is_printable(std::string str) {
	for (char c : str) {
		if (!is_printable(c)) return false;
	}
	return true;
}

bool StringUtils::parseint(std::string str, int *value) {
	size_t pos = 0;
	
	bool sign = false;
	if (str[pos] == '-') {
		sign = true;
		pos++;
	}
	
	if (pos >= str.size()) {
		return false;
	}
	
	*value = 0;
	while (pos < str.size()) {
		if (!isdigit(str[pos])) {
			return false;
		}
		*value = *value * 10 + str[pos] - '0';
		
		if (*value > 99999999) {
			return false;
		}
		
		pos++;
	}
	
	if (sign) {
		*value = -*value;
	}
	return true;
}
