
#pragma once

#include "common/exceptions.h"
#include "common/typeutils.h"
#include "common/stringutils.h"
#include "common/inputstream.h"
#include "math/matrix.h"

#include <initializer_list>
#include <cstring>
#include <cmath>


template <int N>
Vector<N>::Vector() {
	clear();
}

template <int N>
Vector<N>::Vector(InputStream *stream) {
	for (int i = 0; i < N; i++) {
		this->elems[i] = stream->f32();
	}
}

template <int N>
Vector<N>::Vector(std::initializer_list<float> list) {
	if (list.size() != N) {
		runtime_error("Vector initializer list has incorrect size");
	}
	
	int index = 0;
	for (float value : list) {
		this->elems[index] = value;
		index++;
	}
}

template <int N>
std::string Vector<N>::string() const {
	std::string s = "{";
	for (int i = 0; i < N; i++) {
		s += StringUtils::format("%f", this->elems[i]);
		if (i != N - 1) {
			s += ", ";
		}
	}
	s += "}";
	return s;
}
	
template <int N>
void Vector<N>::clear() {
	memset(this->elems, 0, sizeof(this->elems));
}

template <int N>	
template <EnableImpl(N == 3)>
void Vector<N>::rotate(float x, float y, float z) {
	*this = Matrix3f::rotate({x, y, z}) * *this;
}

template <int N>	
float Vector<N>::element(int index) const {
	return this->elems[index];
}

template <int N>	
float &Vector<N>::operator [](int index) {
	return this->elems[index];
}

template <int N>	
const float &Vector<N>::operator [](int index) const {
	return this->elems[index];
}

template <int N>	
float Vector<N>::length() const {
	float sum = 0;
	for (int i = 0; i < N; i++) {
		sum += this->elems[i] * this->elems[i];
	}
	return sqrtf(sum);
}

template <int N>	
Vector<N> Vector<N>::normalized() const {
	return *this / length();
}

template <int N>	
template <EnableImpl(N == 3)>
Vector<N> Vector<N>::cross(const Vector &other) const {
	return Vector({
		this->elems[1] * other[2] - this->elems[2] * other[1],
		this->elems[2] * other[0] - this->elems[0] * other[2],
		this->elems[0] * other[1] - this->elems[1] * other[0]
	});
}

template <int N>
float Vector<N>::dot(const Vector &other) const {
	float value = 0;
	for (int i = 0; i < N; i++) {
		value += this->elems[i] * other[i];
	}
	return value;
}

template <int N>
Vector<N> Vector<N>::operator -() const {
	return *this * -1;
}

template <int N>	
Vector<N> Vector<N>::operator *(float value) const {
	Vector result;
	for (int i = 0; i < N; i++) {
		result[i] = this->elems[i] * value;
	}
	return result;
}

template <int N>
Vector<N> Vector<N>::operator /(float value) const {
	return *this * (1 / value);
}

template <int N>	
Vector<N> Vector<N>::operator +(const Vector &other) const {
	Vector result;
	for (int i = 0; i < N; i++) {
		result[i] = this->elems[i] + other[i];
	}
	return result;
}

template <int N>
Vector<N> Vector<N>::operator -(const Vector &other) const {
	Vector result;
	for (int i = 0; i < N; i++) {
		result[i] = this->elems[i] - other[i];
	}
	return result;
}

template <int N>
Vector<N> &Vector<N>::operator *=(float value) {
	return *this = *this * value;
}

template <int N>
Vector<N> &Vector<N>::operator /=(float value) {
	return *this = *this / value;
}
	
template <int N>
Vector<N> &Vector<N>::operator +=(const Vector &other) {
	return *this = *this + other;
}
	
template <int N>
Vector<N> &Vector<N>::operator -=(const Vector &other) {
	return *this = *this - other;
}

template <int N>
template <EnableImpl(N == 3)>
Vector<N> Vector<N>::normal(const Vector &p1, const Vector &p2, const Vector &p3) {
	return (p2 - p1).cross(p3 - p1).normalized();
}
