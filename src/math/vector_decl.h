
#pragma once

#include "common/typeutils.h"
#include "common/inputstream.h"
#include <initializer_list>
#include <string>


template <int N>
class VectorStorage {
public:
	union {
		struct { float x, y, z, w; };
		float elems[N];
	};
};


template <>
class VectorStorage<3> {
public:
	union {
		struct { float x, y, z; };
		float elems[3];
	};
};


template <>
class VectorStorage<2> {
public:
	union {
		struct { float x, y; };
		float elems[2];
	};
};


template <>
class VectorStorage<1> {
public:
	union {
		struct { float x; };
		float elems[1];
	};
};


template <int N>
class Vector : public VectorStorage<N> {
public:
	Vector();
	Vector(InputStream *stream);
	Vector(std::initializer_list<float> list);
	
	std::string string() const;
	
	void clear();
	
	template <EnableIf(N == 3)>
	void rotate(float x, float y, float z);
	
	float element(int index) const;
	
	float &operator [](int index);
	const float &operator [](int index) const;
	
	float length() const;
	Vector normalized() const;
	
	template <EnableIf(N == 3)>
	Vector cross(const Vector &other) const;
	
	float dot(const Vector &other) const;
	
	Vector operator -() const;
	
	Vector operator *(float value) const;
	Vector operator /(float value) const;
	
	Vector operator +(const Vector &other) const;
	Vector operator -(const Vector &other) const;
	
	Vector &operator *=(float value);
	Vector &operator /=(float value);
	
	Vector &operator +=(const Vector &other);
	Vector &operator -=(const Vector &other);
	
	template <EnableIf(N == 3)>
	static Vector normal(const Vector &p1, const Vector &p2, const Vector &p3);
};

using Vector3f = Vector<3>;
using Vector4f = Vector<4>;
