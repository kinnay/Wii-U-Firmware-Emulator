
#pragma once

#include "math/vector.h"
#include "common/inputstream.h"

class AABox {
public:
	AABox();
	AABox(InputStream *stream);
	
	Vector3f low;
	Vector3f high;
};
