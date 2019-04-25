
#pragma once

#include <cstdint>

const int LineBits = 3;
const int LineNum = 1 << LineBits;
const int LineMask = LineNum - 1;

const int CacheBits = 5;
const int CacheSize = 1 << CacheBits;
const int CacheMask = CacheSize - 1;


class CacheLine {
public:
	bool valid;
	uint8_t data[CacheSize];
	uint32_t addr;
};


class MemoryCache {
public:
	MemoryCache();
	
	void invalidate();

	template <class T>
	bool get(uint32_t addr, T *value) {
		int line = (addr >> CacheBits) & LineMask;
		if (lines[line].valid && lines[line].addr == (addr & ~CacheMask)) {
			*value = *(T *)(&lines[line].data[addr & CacheMask]);
			Endian::swap(value);
			return true;
		}
		return false;
	}
	
	void *alloc(uint32_t addr);
	void enable(uint32_t addr);
	
	bool enabled;

private:
	CacheLine lines[LineNum];
};
