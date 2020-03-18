
#pragma once

#include "common/refcountedobj.h"
#include "common/endian.h"
#include <stack>

class BinaryStream : public RefCountedObj {
public:
	BinaryStream(Endian::Type endian = Endian::Big);
	
	void set_endian(Endian::Type endian);
	
	void push();
	void pop();
	
	virtual size_t size() = 0;
	virtual size_t tell() = 0;
	virtual void seek(size_t pos) = 0;
	
protected:
	bool swap;
	
private:
	std::stack<size_t> stack;
};
