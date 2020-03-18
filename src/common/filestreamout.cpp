
#include "common/filestreamout.h"
#include "common/exceptions.h"
#include <cstdio>

FileStreamOut::FileStreamOut(std::string filename) {
	file = fopen(filename.c_str(), "wb");
	if (!file) {
		runtime_error("Failed to open file");
	}
}

FileStreamOut::~FileStreamOut() {
	fclose(file);
}

size_t FileStreamOut::size() {
	size_t pos = ftell(file);
	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	fseek(file, pos, SEEK_SET);
	return size;
}

size_t FileStreamOut::tell() {
	return ftell(file);
}

void FileStreamOut::seek(size_t pos) {
	if (pos > size()) {
		runtime_error("FileStreamOut::seek overflow");
	}
	fseek(file, pos, SEEK_SET);
}

void FileStreamOut::write(const void *buffer, size_t size) {
	if (fwrite(buffer, 1, size, file) != size) {
		runtime_error("Failed to write to file");
	}
}
