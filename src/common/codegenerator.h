
#pragma once

#include "common/outputtextstream.h"
#include "common/stringstreamout.h"
#include <string>

class CodeGenerator : public OutputTextStream {
public:
	CodeGenerator();
	
	using OutputTextStream::write;
	
	void clear();
	void write(std::string str);
	std::string get();
	
	void indent();
	void unindent();
	
	void beginline();
	void endline();
	
	template <class... Args>
	void writeline(const char *str, Args... args) {
		beginline();
		write(str, args...);
		endline();
	}
	
	void writeline() {
		beginline();
		endline();
	}
	
private:
	StringStreamOut stream;
	int tabs;
};
