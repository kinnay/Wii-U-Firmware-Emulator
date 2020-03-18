
#pragma once

#include "common/outputtextstream.h"
#include "common/inputtextstream.h"
#include "common/refcountedobj.h"

class Sys {
public:
	static Ref<OutputTextStream> stdout;
	static Ref<OutputTextStream> stderr;
	static Ref<InputTextStream> stdin;
};
