
#include "common/stringstreamout.h"

void StringStreamOut::clear() {
	string.clear();
}

void StringStreamOut::write(std::string str) {
	string.append(str);
}

std::string StringStreamOut::get() {
	return string;
}
