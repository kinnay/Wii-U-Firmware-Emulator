
#pragma once

#include <Python.h>
#include "class_sha1.h"

struct SHA1Obj {
	PyObject_HEAD
	SHA1 *object;
};

extern PyTypeObject SHA1Type;
bool SHA1Type_Init();
