
#pragma once

#include "common/inputstream.h"

class FourCC {
public:
	FourCC();
	FourCC(const char *str);
	FourCC(InputStream *stream);
	
	std::string string() const;

	bool operator ==(const FourCC &) const;
	bool operator !=(const FourCC &) const;
	bool operator <(const FourCC &) const;
	
private:
	char chars[4];
};
