
#pragma once

#include "common/inputstream.h"

class Color4f {
public:
	Color4f();
	Color4f(float r, float g, float b, float a);
	Color4f(InputStream *stream);
	
	float r, g, b, a;
};
