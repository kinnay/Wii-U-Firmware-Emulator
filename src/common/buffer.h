
#pragma once

#include "common/refcountedobj.h"
#include <string>
#include <cstdint>


class BufferHolder : public RefCountedObj {
public:
	BufferHolder(size_t size);
	BufferHolder(void *data);
	~BufferHolder();
	
	uint8_t *get();

private:
	uint8_t *data;
};


class Buffer {
public:
	enum Ownership {
		TakeOwnership, CreateCopy
	};
	
	Buffer();
	Buffer(size_t size);
	Buffer(void *data, size_t size, Ownership ownership);
	
	uint8_t *get();
	void read(void *buffer, size_t offset, size_t size);
	void write(const void *buffer, size_t offset, size_t size);
	size_t size();
	
	Buffer slice(size_t start, size_t end);
	
	Buffer copy();
	
	std::string tostring();
	
	std::string hexstring();
	
private:
	Buffer(Ref<BufferHolder> buffer, size_t offset, size_t size);
	
	Ref<BufferHolder> buffer;
	size_t offset;
	size_t length;
};
