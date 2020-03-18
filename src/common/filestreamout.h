
#pragma once

#include "common/refcountedobj.h"
#include "common/outputstream.h"
#include <cstdio>

class FileStreamOut : public OutputStream {
public:
	FileStreamOut(std::string filename);
	~FileStreamOut();
	
	size_t size();
	size_t tell();
	void seek(size_t pos);
	void write(const void *buffer, size_t size);
	using OutputStream::write;
	
private:
	FILE *file;
};
