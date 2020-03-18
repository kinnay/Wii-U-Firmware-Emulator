
#include "common/memorystreamin.h"
#include "common/exceptions.h"
#include <cstring>

MemoryStreamIn::MemoryStreamIn(Buffer buffer, Endian::Type endian) : InputStream(endian) {
	this->buffer = buffer;
	offset = 0;
}

size_t MemoryStreamIn::tell() {
	return offset;
}

void MemoryStreamIn::seek(size_t pos) {
	if (pos > buffer.size()) {
		runtime_error("MemoryStreamIn::seek overflow");
	}
	offset = pos;
}

Buffer MemoryStreamIn::read(size_t size) {
	if (available() < size) {
		runtime_error(
			"MemoryStreamIn: attempted to read %i bytes, while only %i were available",
			size, available()
		);
	}
	size_t start = offset;
	size_t end = offset + size;
	offset += size;
	
	return buffer.slice(start, end);
}

void MemoryStreamIn::read(void *buffer, size_t size) {
	if (available() < size) {
		runtime_error(
			"MemoryStreamIn: attempted to read %i bytes, while only %i were available",
			size, available()
		);
	}
	this->buffer.read(buffer, offset, size);
	offset += size;
}

size_t MemoryStreamIn::size() {
	return buffer.size();
}
