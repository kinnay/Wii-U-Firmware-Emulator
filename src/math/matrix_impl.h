
#pragma once

#include "common/exceptions.h"
#include "common/typeutils.h"
#include "common/stringutils.h"
#include "math/vector_impl.h"

#include <ciso646>
#include <initializer_list>
#include <cmath>

#undef minor


template <int R, int C>
Matrix<R, C>::Matrix() {
	clear();
}

template <int R, int C>
Matrix<R, C>::Matrix(std::initializer_list<float> list) {
	if (list.size() != R * C) {
		runtime_error("Matrix initializer list has incorrect size");
	}
	
	int index = 0;
	for (float elem : list) {
		rows[index / C][index % C] = elem;
		index++;
	}
}

template <int R, int C>
std::string Matrix<R, C>::string() {
	std::string s = "{\n";
	for (int i = 0; i < R; i++) {
		std::string row = "\t";
		for (int j = 0; j < C; j++) {
			row += StringUtils::format("%f", rows[i][j]);
			if (j != C - 1) {
				row += ", ";
			}
		}
		if (i != R - 1) {
			row += ",";
		}
		row += "\n";
		s += row;
	}
	s += "}";
	return s;
}

template <int R, int C>
void Matrix<R, C>::clear() {
	for (int i = 0; i < R; i++) {
		rows[i].clear();
	}
}

template <int R, int C>
Vector<C> Matrix<R, C>::row(int index) const {
	return rows[index];
}

template <int R, int C>
Vector<R> Matrix<R, C>::column(int index) const {
	Vector<R> vec;
	for (int i = 0; i < R; i++) {
		vec[i] = row(i)[index];
	}
	return vec;
}

template <int R, int C>
Vector<C> &Matrix<R, C>::operator [](int row) {
	return rows[row];
}

template <int R, int C>
const Vector<C> &Matrix<R, C>::operator [](int row) const {
	return rows[row];
}

template <int R, int C>
Matrix<R, C> Matrix<R, C>::transposed() {
	Matrix<C, R> m;
	for (int i = 0; i < R; i++) {
		for (int j = 0; j < C; j++) {
			m[j][i] = rows[i][j];
		}
	}
	return m;
}

template <int R, int C>
Matrix<R - 1, C - 1> Matrix<R, C>::submatrix(int row, int column) {
	Matrix<R - 1, C - 1> m;
	
	int r = 0;
	for (int i = 0; i < R; i++) {
		if (i == row) continue;
		
		int c = 0;
		for (int j = 0; j < C; j++) {
			if (j == column) continue;
			
			m[r][c] = rows[i][j];
			c++;
		}
		
		r++;
	}
	
	return m;
}

template <int R, int C>
template <EnableImpl(R == C)>
float Matrix<R, C>::minor(int row, int column) {
	return submatrix(row, column).determinant();
}

template <int R, int C>
template <EnableImpl(R == C)>
Matrix<R, C> Matrix<R, C>::minors() {
	Matrix m;
	for (int i = 0; i < R; i++) {
		for (int j = 0; j < C; j++) {
			m[i][j] = minor(i, j);
		}
	}
	return m;
}

template <int R, int C>
template <EnableImpl(R == C)>
Matrix<R, C> Matrix<R, C>::cofactors() {
	Matrix m = minors();
	for (int i = 0; i < R; i++) {
		for (int j = 0; j < C; j++) {
			if (i % 2 != j % 2) {
				m[i][j] = -m[i][j];
			}
		}
	}
	return m;
}

template <int R, int C>
template <EnableImpl(R == C)>
Matrix<R, C> Matrix<R, C>::adjugate() {
	return cofactors().transposed();
}

template <int R, int C>
template <EnableImpl(R == 2 and C == 2)>
float Matrix<R, C>::determinant() {
	return rows[0][0] * rows[1][1] - rows[0][1] * rows[1][0];
}

template <int R, int C>
template <EnableImpl(R == C and R != 2)>
float Matrix<R, C>::determinant() {
	float det = 0;
	for (int i = 0; i < C; i++) {
		float temp = rows[0][i] * minor(0, i);
		if (i % 2) {
			det -= temp;
		}
		else {
			det += temp;
		}
	}
	return det;
}

template <int R, int C>
Matrix<R, C> Matrix<R, C>::inverse() {
	return adjugate() / determinant();
}

template <int R, int C>
Matrix<R, C> Matrix<R, C>::operator *(float value) {
	Matrix m;
	for (int i = 0; i < R; i++) {
		for (int j = 0; j < C; j++) {
			m[i][j] = rows[i][j] * value;
		}
	}
	return m;
}

template <int R, int C>
Matrix<R, C> Matrix<R, C>::operator /(float value) {
	return *this * (1 / value);
}

template <int R, int C>
template <int N>
Matrix<R, N> Matrix<R, C>::operator *(const Matrix<C, N> &other) {
	Matrix<R, N> result;
	for (int i = 0; i < R; i++) {
		for (int j = 0; j < N; j++) {
			result[i][j] = rows[i].dot(other.column(j));
		}
	}
	return result;
}

template <int R, int C>
Vector<R> Matrix<R, C>::operator *(const Vector<C> &other) {
	Vector<R> result;
	for (int i = 0; i < R; i++) {
		result[i] = rows[i].dot(other);
	}
	return result;
}

template <int R, int C>
template <EnableImpl(R == C)>
Matrix<R, C> Matrix<R, C>::identity() {
	Matrix m;
	for (int i = 0; i < R; i++) {
		m[i][i] = 1;
	}
	return m;
}

template <int R, int C>
template <EnableImpl(R == C)>
Matrix<R, C> Matrix<R, C>::translate(Vector<R - 1> trans) {
	Matrix m = identity();
	for (int i = 0; i < R - 1; i++) {
		m[i][C - 1] = trans[i];
	}
	return m;
}

template <int R, int C>
template <EnableImpl(R == C)>
Matrix<R, C> Matrix<R, C>::scale(Vector<R - 1> scale) {
	Matrix m;
	for (int i = 0; i < R - 1; i++) {
		m[i][i] = scale[i];
	}
	m[R - 1][C - 1] = 1;
	return m;
}

template <int R, int C>
template <EnableImpl((R == 3 and C == 3) or (R == 4 and C == 4))>
Matrix<R, C> Matrix<R, C>::rotate_axis(float angle, Vector3f axis) {
	Matrix m = identity();
	
	axis = axis.normalized();
	
	float x = axis.x;
	float y = axis.y;
	float z = axis.z;
	
	float cos = cosf(angle);
	float sin = sinf(angle);
	
	m[0][0] = cos + x * x * (1 - cos);
	m[0][1] = x * y * (1 - cos) - z * sin;
	m[0][2] = x * z * (1 - cos) + y * sin;
	m[1][0] = y * x * (1 - cos) + z * sin;
	m[1][1] = cos + y * y * (1 - cos);
	m[1][2] = y * z * (1 - cos) - x * sin;
	m[2][0] = z * x * (1 - cos) - y * sin;
	m[2][1] = z * y * (1 - cos) + x * sin;
	m[2][2] = cos + z * z * (1 - cos);
	
	return m;
}

template <int R, int C>
template <EnableImpl((R == 3 and C == 3) or (R == 4 and C == 4))>
Matrix<R, C> Matrix<R, C>::rotate(Vector3f rot) {
	return rotate_axis(rot[0], {1, 0, 0}) * 
	       rotate_axis(rot[1], {0, 1, 0}) *
	       rotate_axis(rot[2], {0, 0, 1});
}

template <int R, int C>
template <EnableImpl((R == 3 and C == 3) or (R == 4 and C == 4))>
Matrix<R, C> Matrix<R, C>::lookat(Vector3f base, Vector3f target) {
	base = base.normalized();
	target = target.normalized();
	
	float angle = acosf(base.dot(target));
	Vector3f axis = base.cross(target).normalized();
	return rotate_axis(angle, axis);
}

template <int R, int C>
template <EnableImpl(R == 4 and C == 4)>
Matrix<R, C> Matrix<R, C>::perspective(float angle, float ratio, float near, float far) {
	float f = 1 / tanf(angle / 2);
	float z1 = (near + far) / (near - far);
	float z2 = 2 * far * near / (near - far);
	
	return Matrix({
		f / ratio, 0, 0, 0,
		0, f, 0, 0,
		0, 0, z1, z2,
		0, 0, -1, 0
	});
}
