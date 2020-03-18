
#include "math/lookatcamera.h"

Matrix4f LookAtCamera::matrix() {
	Vector3f forward = (target - position).normalized();
	Vector3f right = forward.cross({0, 1, 0}).normalized();
	Vector3f up = right.cross(forward);
	
	Matrix4f m = {
		right.x, right.y, right.z, 0,
		up.x, up.y, up.z, 0,
		-forward.x, -forward.y, -forward.z, 0,
		0, 0, 0, 1
	};
	return m * Matrix4f::translate(-position);
}
