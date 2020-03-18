
#pragma once

#include "common/refcountedobj.h"
#include <string>

class InputTextStream : public RefCountedObj {
public:
	std::string readline();
	
	virtual char read() = 0;
};
