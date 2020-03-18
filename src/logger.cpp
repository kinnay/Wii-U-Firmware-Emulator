
#include "logger.h"

#include "common/sys.h"


void ConsoleLogger::init(std::string name) {
	this->name = name;
}

void ConsoleLogger::write(std::string text) {
	buffer += text;
	
	size_t pos = buffer.find('\n');
	while (pos != std::string::npos) {
		std::string message = buffer.substr(0, pos);
		Sys::stdout->write("%s> %s\n", name, message);
		buffer.erase(0, pos + 1);
		pos = buffer.find('\n');
	}
}


void FileLogger::init(std::string filename) {
	stream = new FileStreamOut(filename);
}

void FileLogger::write(std::string text) {
	stream->write(text.c_str(), text.size());
}
