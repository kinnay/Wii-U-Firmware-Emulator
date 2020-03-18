
#pragma once

#include "math/itransform.h"
#include "math/matrix.h"

class PerspectiveProjection : public ITransform {
public:
	PerspectiveProjection();
	void set_aspect_ratio(float ratio);
	void set_angle(float angle);
	void set_clip(float near, float far);
	Matrix4f matrix();
	
private:
	float aspect_ratio;
	float angle;
	float near;
	float far;
};
