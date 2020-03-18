
#include "common/filestreamin.h"
#include "common/filestreamout.h"
#include "common/fileutils.h"
#include "common/buffer.h"

#include <cstdio>

Buffer FileUtils::load(std::string filename) {
	FileStreamIn stream(filename);
	return stream.read(stream.size());
}

void FileUtils::save(std::string filename, Buffer data) {
	FileStreamOut stream(filename);
	stream.write(data);
}
