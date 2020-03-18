
#include "common/inputstream.h"
#include "common/exceptions.h"

template <>
std::string InputStream::read<std::string>() {
	std::string s;
	
	char c = u8();
	while (c) {
		s.push_back(c);
		c = u8();
	}
	return s;
}

int8_t InputStream::s8() { return read<int8_t>(); }
int16_t InputStream::s16() { return read<int16_t>(); }
int32_t InputStream::s32() { return read<int32_t>(); }
int64_t InputStream::s64() { return read<int64_t>(); }

uint8_t InputStream::u8() { return read<uint8_t>(); }
uint16_t InputStream::u16() { return read<uint16_t>(); }
uint32_t InputStream::u32() { return read<uint32_t>(); }
uint64_t InputStream::u64() { return read<uint64_t>(); }

float InputStream::f32() { return read<float>(); }
double InputStream::f64() { return read<double>(); }

bool InputStream::boolean() { return u8() != 0; }

std::string InputStream::string() { return read<std::string>(); }

std::string InputStream::ascii(size_t size) {
	std::string s;
	for (size_t i = 0; i < size; i++) {
		s.push_back(u8());
	}
	return s;
}

bool InputStream::eof() {
	return available() == 0;
}

size_t InputStream::available() {
	return size() - tell();
}

void InputStream::skip(size_t size) {
	if (available() < size) {
		runtime_error(
			"InputStream: attempted to skip %i bytes, while only %i were available.",
			size, available()
		);
	}
	seek(tell() + size);
}

Buffer InputStream::read(size_t size) {
	Buffer buffer(size);
	read(buffer.get(), size);
	return buffer;
}
