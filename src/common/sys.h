
#pragma once

#include "common/outputtextstream.h"
#include "common/inputtextstream.h"
#include "common/refcountedobj.h"

class Sys {
public:
	static Ref<OutputTextStream> out;
	static Ref<OutputTextStream> err;
	static Ref<InputTextStream> in;
};
