
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

Ref<OutputTextStream> Sys::stdout = new StdOut();
Ref<OutputTextStream> Sys::stderr = new StdErr();
Ref<InputTextStream> Sys::stdin = new StdIn();
