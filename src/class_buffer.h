
#pragma once

#include <cstdint>

class Buffer {
	public:
	Buffer();
	~Buffer();
	bool init(uint32_t size);
	bool init(const void *data, uint32_t size);
	void *raw();
	
	template<class T>
	T *ptr(uint32_t offs) {
		if (offs + sizeof(T) > size) {
			OverflowError("Attempted to read outside of buffer");
			return NULL;
		}
		return (T *)((char *)data + offs);
	}
	
	private:
	void *data;
	uint32_t size;
};
