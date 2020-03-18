
#include "common/stringstreamin.h"
#include "common/exceptions.h"

StringStreamIn::StringStreamIn(std::string str) {
	this->str = str;
	pos = 0;
}

bool StringStreamIn::eof() {
	return pos >= str.size();
}

char StringStreamIn::read() {
	if (eof()) {
		runtime_error("No more characters available");
	}
	return str.at(pos++);
}
