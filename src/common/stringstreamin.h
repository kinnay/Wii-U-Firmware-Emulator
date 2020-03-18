
#pragma once

#include "common/inputtextstream.h"

class StringStreamIn : public InputTextStream {
public:
	StringStreamIn(std::string str);
	bool eof();
	char read();
	
private:
	std::string str;
	size_t pos;
};
