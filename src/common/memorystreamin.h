
#pragma once

#include "common/inputstream.h"
#include "common/endian.h"

class MemoryStreamIn : public InputStream {
public:
	MemoryStreamIn(Buffer buffer, Endian::Type endian = Endian::Big);

	Buffer read(size_t size);

	size_t size();
	size_t tell();
	void seek(size_t pos);
	void read(void *buffer, size_t size);
	
private:
	Buffer buffer;
	size_t offset;
};
