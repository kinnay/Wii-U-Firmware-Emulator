
#include "common/refcountedobj.h"
#include <cstdio>
#include <cstdlib>
#include <stack>

void *_last_allocated = nullptr;

RefCountedObj::RefCountedObj() {
	heap = false;
	if (_last_allocated == this) {
		_last_allocated = nullptr;
		heap = true;
	}
	
	refs = 0;
}

RefCountedObj::RefCountedObj(const RefCountedObj &other) : RefCountedObj() {}

RefCountedObj::~RefCountedObj() {
	if (refs > 0) {
		fprintf(stderr, "Attempting to delete object at %p with %i refs\n", this, refs);
		abort();
	}
}

RefCountedObj &RefCountedObj::operator =(const RefCountedObj &other) {
	return *this;
}

void *RefCountedObj::operator new(size_t size) {
	void *buffer = ::operator new(size);
	_last_allocated = buffer;
	return buffer;
}

void RefCountedObj::inc_refs() {
	refs++;
}

void RefCountedObj::dec_refs() {
	refs--;
	if (refs <= 0) {
		if (heap) {
			delete this;
		}
	}
}
