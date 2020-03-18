
#pragma once

#include "math/itransform.h"
#include "math/vector.h"

class LookAtCamera : public ITransform {
public:
	Matrix4f matrix();

	Vector3f position;
	Vector3f target;
};
