
#include "class_buffer.h"
#include "errors.h"
#include <cstdint>
#include <cstring>

Buffer::Buffer() : data(NULL), size(0) {}

Buffer::~Buffer() {
	if (data) {
		delete data;
	}
}

bool Buffer::init(uint32_t size) {
	data = new char[size];
	if (!data) {
		MemoryError("Couldn't allocate memory buffer (size: 0x%x)\n", size);
		return false;
	}
	this->size = size;
	return true;
}

bool Buffer::init(const void *data, uint32_t size) {
	if (!init(size)) return false;
	memcpy(this->data, data, size);
	return true;
	
}

void *Buffer::raw() {
	return data;
}
