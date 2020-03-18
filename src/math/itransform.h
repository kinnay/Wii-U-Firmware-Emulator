
#pragma once

#include "math/matrix.h"
#include "common/refcountedobj.h"

class ITransform : public RefCountedObj {
public:
	virtual ~ITransform() {}
	virtual Matrix4f matrix() = 0;
};
