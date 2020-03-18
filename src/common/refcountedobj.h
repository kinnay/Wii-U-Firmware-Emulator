
#pragma once

#include "common/exceptions.h"
#include <cstddef>

class RefCountedObj {
public:
	RefCountedObj();
	virtual ~RefCountedObj();
	
	RefCountedObj(const RefCountedObj &other);
	RefCountedObj &operator =(const RefCountedObj &other);
	
	void inc_refs();
	void dec_refs();
	
	void *operator new(size_t size);
	
private:
	int refs;
	bool heap;
};


template <class T>
class Ref {
public:
	Ref() {
		ptr = nullptr;
	}
	
	Ref(T *ptr) {
		this->ptr = ptr;
		if (ptr) {
			ptr->inc_refs();
		}
	}
	
	Ref(const Ref &other) {
		ptr = other.ptr;
		if (ptr) {
			ptr->inc_refs();
		}
	}
	
	~Ref() {
		if (ptr) {
			ptr->dec_refs();
		}
	}
	
	template <class C>
	Ref<C> cast() {
		Ref<C> casted = dynamic_cast<C *>(ptr);
		if (!casted) {
			runtime_error("Failed to cast object");
		}
		return casted;
	}
	
	template <class C>
	operator Ref<C> () const {
		return ptr;
	}
	
	Ref &operator =(const Ref &other) {
		if (other.ptr) {
			other.ptr->inc_refs();
		}
		if (this->ptr) {
			this->ptr->dec_refs();
		}
		this->ptr = other.ptr;
		return *this;
	}
	
	T *operator ->() {
		if (!ptr) {
			runtime_error("Null pointer dereference");
		}
		return ptr;
	}
	
	operator T *() const {
		return ptr;
	}
	
private:
	T *ptr;
};
