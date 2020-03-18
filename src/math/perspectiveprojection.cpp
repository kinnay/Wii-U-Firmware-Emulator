
#include "math/perspectiveprojection.h"
#include "math/math.h"

PerspectiveProjection::PerspectiveProjection() {
	aspect_ratio = 1;
	angle = radians(45);
	near = 0.1f;
	far = 1000;
}

void PerspectiveProjection::set_aspect_ratio(float ratio) {
	aspect_ratio = ratio;
}

void PerspectiveProjection::set_angle(float angle) {
	this->angle = angle;
}

void PerspectiveProjection::set_clip(float near, float far) {
	this->near = near;
	this->far = far;
}

Matrix4f PerspectiveProjection::matrix() {
	return Matrix4f::perspective(angle, aspect_ratio, near, far);
}
