
#include "common/filestreamin.h"
#include "common/exceptions.h"
#include <cstdio>

FileStreamIn::FileStreamIn(std::string filename) {
	file = fopen(filename.c_str(), "rb");
	if (!file) {
		runtime_error("Failed to open file: %s", filename);
	}
	
	fseek(file, 0, SEEK_END);
	filesize = ftell(file);
	rewind(file);
}

FileStreamIn::~FileStreamIn() {
	fclose(file);
}

size_t FileStreamIn::size() {
	return filesize;
}

size_t FileStreamIn::tell() {
	return ftell(file);
}

void FileStreamIn::seek(size_t pos) {
	if (pos > filesize) {
		runtime_error("FileStreamIn::seek overflow");
	}
	fseek(file, pos, SEEK_SET);
}

void FileStreamIn::read(void *buffer, size_t size) {
	if (fread(buffer, 1, size, file) != size) {
		runtime_error("FileStreamIn::read overflow");
	}
}
