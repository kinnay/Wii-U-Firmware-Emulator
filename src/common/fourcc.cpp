
#include "common/exceptions.h"
#include "common/fourcc.h"
#include <cstring>

FourCC::FourCC() {
	chars[0] = 0;
	chars[1] = 0;
	chars[2] = 0;
	chars[3] = 0;
}

FourCC::FourCC(const char *str) {
	if (strlen(str) != 4) {
		runtime_error("FourCC constructed with invalid string");
	}
	chars[0] = str[0];
	chars[1] = str[1];
	chars[2] = str[2];
	chars[3] = str[3];
}

FourCC::FourCC(InputStream *stream) {
	chars[0] = stream->u8();
	chars[1] = stream->u8();
	chars[2] = stream->u8();
	chars[3] = stream->u8();
}

std::string FourCC::string() const {
	std::string s;
	s += chars[0];
	s += chars[1];
	s += chars[2];
	s += chars[3];
	return s;
}

bool FourCC::operator ==(const FourCC &other) const {
	for (int i = 0; i < 4; i++) {
		if (chars[i] != other.chars[i]) {
			return false;
		}
	}
	return true;
}

bool FourCC::operator !=(const FourCC &other) const {
	return !(*this == other);
}

bool FourCC::operator <(const FourCC &other) const {
	for (int i = 0; i < 4; i++) {
		if (chars[i] < other.chars[i]) return true;
		if (chars[i] > other.chars[i]) return false;
	}
	return false;
}
