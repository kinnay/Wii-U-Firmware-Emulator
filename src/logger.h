
#pragma once

#include <string>

#include "common/filestreamout.h"


class ConsoleLogger {
public:
	void init(std::string name);
	void write(std::string text);
	
private:
	std::string name;
	std::string buffer;
};


class FileLogger {
public:
	void init(std::string name);
	void write(std::string text);
	
private:
	Ref<FileStreamOut> stream;
};
