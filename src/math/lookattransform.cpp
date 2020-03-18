
#include "math/lookattransform.h"
#include "math/vector.h"
#include "math/matrix.h"
#include "math/math.h"
#include "common/logger.h"

LookAtTransform::LookAtTransform() {
	basedir = {0, 0, 1};
}

Matrix4f LookAtTransform::matrix() {
	return Matrix4f::translate(position) * Matrix4f::lookat(basedir, target - position);
}
