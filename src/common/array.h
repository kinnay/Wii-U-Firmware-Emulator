
#pragma once

#include "common/exceptions.h"

template <class T>
class Array {
public:
	Array(Buffer buffer) {
		this->buffer = buffer;
	}
	
	T &operator [](int index) {
		int limit = buffer.size() / sizeof(T);
		if (index >= limit) {
			runtime_error(
				"Array index out of bounds: %i >= %i (%i / %i)\n",
				index, limit, buffer.size(), sizeof(T)
			);
		}
		return ((T *)buffer.get())[index];
	}
	
private:
	Buffer buffer;
};
