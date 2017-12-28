
#include "class_filemgr.h"
#include "errors.h"

#include <cstdio>
#include <cstdint>

Buffer *FileMgr::load(const char *filename) {
	FILE *file = fopen(filename, "rb");
	if (!file) {
		IOError("Couldn't open file %s", filename);
		return NULL;
	}
	
	fseek(file, 0, SEEK_END);
	uint32_t filesize = ftell(file);
	
	Buffer *buffer = new Buffer;
	if (!buffer) {
		MemoryError("Couldn't allocate buffer object");
		fclose(file);
		return NULL;
	}
	
	if (!buffer->init(filesize)) {
		MemoryError("Couldn't allocate file buffer (filesize: 0x%x)", filesize);
		fclose(file);
		delete buffer;
		return NULL;
	}
	
	fseek(file, 0, SEEK_SET);
	if (fread(buffer->raw(), 1, filesize, file) != filesize) {
		IOError("An error occurred while reading the file");
		fclose(file);
		delete buffer;
		return NULL;
	}
	
	fclose(file);
	return buffer;
}
