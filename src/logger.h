
#pragma once

#include "common/filestreamout.h"

#include <string>


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
