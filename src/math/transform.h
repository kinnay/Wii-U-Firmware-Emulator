
#pragma once

#include "math/itransform.h"
#include "math/matrix.h"

class Transform : public ITransform {
public:
	Transform();
	void reset();
	void move(float x, float y, float z);
	void rotate(float x, float y, float z);
	void scale(float x, float y, float z);
	Matrix4f matrix();

private:
	Matrix4f mtx;
};
