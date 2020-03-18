
#pragma once

#include "common/outputtextstream.h"
#include <string>

class StringStreamOut : public OutputTextStream {
public:
	void clear();
	void write(std::string str);
	std::string get();
	
private:
	std::string string;
};
