
#pragma once

#include "common/refcountedobj.h"
#include "common/inputstream.h"
#include <string>
#include <cstdio>

class FileStreamIn : public InputStream {
public:
	FileStreamIn(std::string filename);
	~FileStreamIn();
	
	size_t size();
	size_t tell();
	void seek(size_t pos);
	void read(void *buffer, size_t size);
	using InputStream::read;
	
private:
	FILE *file;
	size_t filesize;
};
