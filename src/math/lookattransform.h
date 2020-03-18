
#pragma once

#include "math/itransform.h"
#include "math/matrix.h"
#include "math/vector.h"

class LookAtTransform : public ITransform {
public:
	LookAtTransform();
	
	Matrix4f matrix();
	
	Vector3f basedir;
	Vector3f position;
	Vector3f target;
};
