
#pragma once

#include "common/refcountedobj.h"
#include "common/stringutils.h"

class OutputTextStream : public RefCountedObj {
public:
	template <class... Args>
	void write(const char *str, Args... args) {
		std::string s = StringUtils::format(str, args...);
		write(s);
	}
	
	virtual void write(std::string str) = 0;
};
