
#include "common/sha1.h"
#include "common/endian.h"

static uint32_t rol(uint32_t value, int bits) {
	return (value << bits) | (value >> (32 - bits));
}

void SHA1::update(void *data) {
	uint32_t w[80];
	for (int i = 0; i < 16; i++) {
		w[i] = Endian::swap32(((uint32_t *)data)[i]);
	}
	
	for (int i = 16; i < 80; i++) {
		w[i] = rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
	}
	
	uint32_t a = h0;
	uint32_t b = h1;
	uint32_t c = h2;
	uint32_t d = h3;
	uint32_t e = h4;
	
	uint32_t f, k, temp;
	for (int i = 0; i < 80; i++) {
		if (i < 20) {
			f = (b & c) | (~b & d);
			k = 0x5A827999;
		}
		else if (i < 40) {
			f = b ^ c ^ d;
			k = 0x6ED9EBA1;
		}
		else if (i < 60) {
			f = (b & c) | (b & d) | (c & d);
			k = 0x8F1BBCDC;
		}
		else {
			f = b ^ c ^ d;
			k = 0xCA62C1D6;
		}
		
		temp = rol(a, 5) + f + e + k + w[i];
		e = d;
		d = c;
		c = rol(b, 30);
		b = a;
		a = temp;
	}
	
	h0 += a;
	h1 += b;
	h2 += c;
	h3 += d;
	h4 += e;
}
