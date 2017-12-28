
#pragma once

#include "class_buffer.h"
#include <cstdint>
#include <cstdio>

class FileMgr {
	public:
	static Buffer *load(const char *filename);
};
