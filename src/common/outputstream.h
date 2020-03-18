
#pragma once

#include "common/binarystream.h"
#include "common/buffer.h"
#include "common/endian.h"
#include <string>

class OutputStream : public BinaryStream {
public:
	template <class T>
	void write(T value) {
		if (swap) {
			Endian::swap(&value);
		}
		write(&value, sizeof(T));
	}

	using BinaryStream::BinaryStream;
	
	void s8(int8_t value);
	void s16(int16_t value);
	void s32(int32_t value);
	void s64(int64_t value);
	
	void u8(int8_t value);
	void u16(int16_t value);
	void u32(int32_t value);
	void u64(int64_t value);
	
	void f32(float value);
	void f64(double value);
	
	void boolean(bool value);
	
	void string(std::string value);
	void ascii(std::string value);
	
	void write(Buffer buffer);
	
	virtual void write(const void *buffer, size_t size) = 0;
};
