
#include "common/binarystream.h"

BinaryStream::BinaryStream(Endian::Type endian) {
	swap = endian == Endian::Big;
}

void BinaryStream::set_endian(Endian::Type endian) {
	swap = endian == Endian::Big;
}

void BinaryStream::push() {
	stack.push(tell());
}

void BinaryStream::pop() {
	seek(stack.top());
	stack.pop();
}
