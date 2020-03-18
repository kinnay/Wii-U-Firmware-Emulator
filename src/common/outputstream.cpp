
#include "common/outputstream.h"

void OutputStream::s8(int8_t value) { write(value); }
void OutputStream::s16(int16_t value) { write(value); }
void OutputStream::s32(int32_t value) { write(value); }
void OutputStream::s64(int64_t value) { write(value); }

void OutputStream::u8(int8_t value) { write(value); }
void OutputStream::u16(int16_t value) { write(value); }
void OutputStream::u32(int32_t value) { write(value); }
void OutputStream::u64(int64_t value) { write(value); }

void OutputStream::f32(float value) { write(value); }
void OutputStream::f64(double value) { write(value); }

void OutputStream::boolean(bool value) { u8(value); }

void OutputStream::string(std::string value) {
	ascii(value);
	u8(0);
}

void OutputStream::ascii(std::string value) {
	write(value.c_str(), value.size());
}

void OutputStream::write(Buffer buffer) {
	write(buffer.get(), buffer.size());
}
