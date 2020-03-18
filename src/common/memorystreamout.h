
#pragma once

#include "common/outputstream.h"
#include "common/endian.h"

class MemoryStreamOut : public OutputStream {
public:
	MemoryStreamOut(Buffer buffer, Endian::Type endian = Endian::Big);
	
	size_t size();
	size_t tell();
	void seek(size_t pos);
	void write(const void *buffer, size_t size);
	using OutputStream::write;
	
private:
	Buffer buffer;
	size_t offset;
};
