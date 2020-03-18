
#pragma once

#include "common/buffer.h"
#include <string>

class FileUtils {
public:
	static Buffer load(std::string filename);
	static void save(std::string filename, Buffer data);
};
