
#include "common/memorystreamout.h"
#include "common/exceptions.h"

MemoryStreamOut::MemoryStreamOut(Buffer buffer, Endian::Type endian) : OutputStream(endian) {
	this->buffer = buffer;
	offset = 0;
}

size_t MemoryStreamOut::size() {
	return buffer.size();
}

size_t MemoryStreamOut::tell() {
	return offset;
}

void MemoryStreamOut::seek(size_t pos) {
	if (pos > size()) {
		runtime_error("MemoryStreamOut::seek overflow");
	}
	offset = pos;
}

void MemoryStreamOut::write(const void *buffer, size_t size) {
	this->buffer.write(buffer, offset, size);
	offset += size;
}
