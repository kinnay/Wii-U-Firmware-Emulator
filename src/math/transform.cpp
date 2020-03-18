
#include "math/transform.h"

Transform::Transform() {
	reset();
}

void Transform::reset() {
	mtx = Matrix4f::identity();
}

void Transform::move(float x, float y, float z) {
	mtx = Matrix4f::translate({x, y, z}) * mtx;
}

void Transform::rotate(float x, float y, float z) {
	mtx = Matrix4f::rotate({x, y, z}) * mtx;
}

void Transform::scale(float x, float y, float z) {
	mtx = Matrix4f::scale({x, y, z}) * mtx;
}

Matrix4f Transform::matrix() {
	return mtx;
}
