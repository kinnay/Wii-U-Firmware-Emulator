
#include "common/sys.h"
#include "common/outputtextstream.h"
#include "common/inputtextstream.h"
#include <cstdio>

class StdOut : public OutputTextStream {
public:
	void write(std::string str) {
		fputs(str.c_str(), stdout);
	}
};

class StdErr : public OutputTextStream {
public:
	void write(std::string str) {
		fputs(str.c_str(), stderr);
	}
};

class StdIn : public InputTextStream {
public:
	char read() {
		return getchar();
	}
};

Ref<OutputTextStream> Sys::out = new StdOut();
Ref<OutputTextStream> Sys::err = new StdErr();
Ref<InputTextStream> Sys::in = new StdIn();
