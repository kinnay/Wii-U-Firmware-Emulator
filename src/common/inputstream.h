
#pragma once

#include "common/binarystream.h"
#include "common/buffer.h"
#include "common/endian.h"
#include <string>

class InputStream : public BinaryStream {
public:
	template <class T>
	T read() {
		T value;
		read(&value, sizeof(T));
		if (swap) {
			Endian::swap(&value);
		}
		return value;
	}
	
	using BinaryStream::BinaryStream;
	
	int8_t s8();
	int16_t s16();
	int32_t s32();
	int64_t s64();
	
	uint8_t u8();
	uint16_t u16();
	uint32_t u32();
	uint64_t u64();
	
	float f32();
	double f64();
	
	bool boolean();
	
	std::string string();
	std::string ascii(size_t size);

	size_t available();
	bool eof();
	
	void skip(size_t size);

	virtual Buffer read(size_t size);
	
	virtual void read(void *buffer, size_t size) = 0;
};
