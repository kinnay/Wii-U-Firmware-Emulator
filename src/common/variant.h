
#pragma once

#include "common/refcountedobj.h"
#include "common/exceptions.h"
#include "math/vector.h"
#include "math/matrix.h"
#include <string>
#include <cstdint>


class Variant : public RefCountedObj {
public:
	enum Type {
		NONE,
		FLOAT,
		MATRIX,
		VECTOR3,
		VECTOR4,
		STRING,
		OBJECT
	};

	Variant();
	Variant(const Variant &value);
	~Variant();
	
	Variant(float value);
	Variant(const Matrix4f &value);
	Variant(const Vector3f &value);
	Variant(const Vector4f &value);
	Variant(const std::string &value);
	
	template <class T>
	Variant(Ref<T> value) {
		type = OBJECT;
		object = value;
		object->inc_refs();
	}
	
	Variant &operator =(const Variant &other);
	
	operator float() const;
	operator const Matrix4f &() const;
	operator const Vector3f &() const;
	operator const Vector4f &() const;
	operator const std::string &() const;
	
	template <class T>
	operator T *() const {
		check_type(OBJECT);
		T *ptr = dynamic_cast<T *>(object);
		if (!ptr) {
			logic_error("Variant has wrong type");
		}
		return ptr;
	}
	
private:
	void check_type(Type type) const;

	Type type;
	union {
		float flt;
		Matrix4f matrix;
		Vector3f vector3f;
		Vector4f vector4f;
		std::string *string;
		RefCountedObj *object;
	};
};
