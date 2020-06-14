
#pragma once

#include "common/typeutils.h"
#include "math/vector_decl.h"

#include <ciso646>
#include <initializer_list>
#include <string>

#undef minor

template <int R, int C>
class Matrix {
public:
	Matrix();
	Matrix(std::initializer_list<float> list);
	
	std::string string();
	
	void clear();
	
	Vector<C> row(int index) const;
	Vector<R> column(int index) const;
	
	Vector<C> &operator [](int row);
	const Vector<C> &operator [](int row) const;
	
	Matrix transposed();
	
	Matrix<R - 1, C - 1> submatrix(int row, int column);
	
	template <EnableIf(R == C)> float minor(int row, int column);
	template <EnableIf(R == C)> Matrix minors();
	template <EnableIf(R == C)> Matrix cofactors();
	template <EnableIf(R == C)> Matrix adjugate();
	
	template <EnableIf(R == 2 and C == 2)>
	float determinant();
	
	template <EnableIf(R == C and R != 2)>
	float determinant();
	
	Matrix inverse();
	
	Matrix operator *(float value);
	Matrix operator /(float value);
	
	template <int N>
	Matrix<R, N> operator *(const Matrix<C, N> &other);
	
	Vector<R> operator *(const Vector<C> &other);
	
	template <EnableIf(R == C)> static Matrix identity();
	template <EnableIf(R == C)> static Matrix translate(Vector<R - 1> trans);
	template <EnableIf(R == C)> static Matrix scale(Vector<R - 1> scale);
	
	template <EnableIf((R == 3 and C == 3) or (R == 4 and C == 4))>
	static Matrix rotate_axis(float angle, Vector3f axis);

	template <EnableIf((R == 3 and C == 3) or (R == 4 and C == 4))>
	static Matrix rotate(Vector3f rot);
	
	template <EnableIf((R == 3 and C == 3) or (R == 4 and C == 4))>
	static Matrix lookat(Vector3f base, Vector3f target);
	
	template <EnableIf(R == 4 and C == 4)>
	static Matrix perspective(float angle, float ratio, float near, float far);
	
private:
	Vector<C> rows[R];
};

using Matrix3f = Matrix<3, 3>;
using Matrix4f = Matrix<4, 4>;
