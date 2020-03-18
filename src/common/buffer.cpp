
#include "common/buffer.h"
#include "common/exceptions.h"
#include "common/stringutils.h"
#include <cstring>

BufferHolder::BufferHolder(size_t size) {
	data = new uint8_t[size];
}

BufferHolder::BufferHolder(void *data) {
	this->data = (uint8_t *)data;
}

BufferHolder::~BufferHolder() {
	delete[] data;
}

uint8_t *BufferHolder::get() {
	return data;
}


Buffer::Buffer() : Buffer(0) {}

Buffer::Buffer(size_t size) {
	if (size) {
		buffer = new BufferHolder(size);
	}
	offset = 0;
	length = size;
}

Buffer::Buffer(void *data, size_t size, Ownership ownership) {
	if (size) {
		if (ownership == TakeOwnership) {
			buffer = new BufferHolder(data);
		}
		else {
			buffer = new BufferHolder(size);
			memcpy(buffer->get(), data, size);
		}
	}
	offset = 0;
	length = size;
}

Buffer::Buffer(Ref<BufferHolder> buffer, size_t offset, size_t size) {
	this->buffer = buffer;
	this->offset = offset;
	this->length = size;
}

uint8_t *Buffer::get() {
	if (buffer) {
		return buffer->get() + offset;
	}
	return nullptr;
}

size_t Buffer::size() {
	return length;
}

void Buffer::read(void *buffer, size_t offset, size_t size) {
	if (offset > length || length - offset < size) {
		runtime_error("Overflow in Buffer::read");
	}
	if (this->buffer) {
		void *base = this->buffer->get() + this->offset + offset;
		memcpy(buffer, base, size);
	}
}

void Buffer::write(const void *buffer, size_t offset, size_t size) {
	if (offset > length || length - offset < size) {
		runtime_error("Overflow in Buffer::write");
	}
	if (this->buffer) {
		void *base = this->buffer->get() + this->offset + offset;
		memcpy(base, buffer, size);
	}
}

Buffer Buffer::slice(size_t start, size_t end) {
	if (end < start || start > length || end > length) {
		runtime_error("Buffer::slice(%i, %i): overflow (length=%i)", start, end, length);
	}
	return Buffer(buffer, offset + start, end - start);
}

Buffer Buffer::copy() {
	return Buffer(get(), size(), CreateCopy);
}

std::string Buffer::tostring() {
	return std::string((const char *)get(), size());
}

std::string Buffer::hexstring() {
	std::string s;
	for (size_t i = 0; i < length; i++) {
		s += StringUtils::format("%02x", buffer->get()[offset + i]);
	}
	return s;
}
