
#include "common/inputtextstream.h"

std::string InputTextStream::readline() {
	std::string str;
	
	char c = read();
	while (c != '\n') {
		str.push_back(c);
		c = read();
	}
	
	return str;
}
