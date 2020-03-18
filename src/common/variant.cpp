
#include "common/variant.h"
#include "common/exceptions.h"

Variant::Variant() {
	type = NONE;
}

Variant::Variant(const Variant &other) {
	type = NONE;
	*this = other;
}

Variant::Variant(float value) {
	type = FLOAT;
	flt = value;
}

Variant::Variant(const Matrix4f &value) {
	type = MATRIX;
	matrix = value;
}

Variant::Variant(const Vector3f &value) {
	type = VECTOR3;
	vector3f = value;
}

Variant::Variant(const Vector4f &value) {
	type = VECTOR4;
	vector4f = value;
}

Variant::Variant(const std::string &value) {
	type = STRING;
	string = new std::string(value);
}

Variant::~Variant() {
	if (type == OBJECT) {
		object->dec_refs();
	}
	else if (type == STRING) {
		delete string;
	}
}

Variant &Variant::operator =(const Variant &other) {
	if (type == OBJECT) {
		object->dec_refs();
	}
	else if (type == STRING) {
		delete string;
	}
	
	type = other.type;
	if (type == FLOAT) flt = other.flt;
	else if (type == MATRIX) matrix = other.matrix;
	else if (type == VECTOR3) vector3f = other.vector3f;
	else if (type == VECTOR4) vector4f = other.vector4f;
	else if (type == STRING) string = new std::string(*other.string);
	else if (type == OBJECT) {
		object = other.object;
		object->inc_refs();
	}
	return *this;
}

void Variant::check_type(Type type) const {
	if (this->type != type) {
		logic_error("Variant has wrong type");
	}
}

Variant::operator float() const {
	check_type(FLOAT);
	return flt;
}

Variant::operator const Matrix4f &() const {
	check_type(MATRIX);
	return matrix;
}

Variant::operator const Vector3f &() const {
	check_type(VECTOR3);
	return vector3f;
}

Variant::operator const Vector4f &() const {
	check_type(VECTOR4);
	return vector4f;
}

Variant::operator const std::string &() const {
	check_type(STRING);
	return *string;
}
