
#pragma once

#include <Python.h>
#include "interface_physmem.h"

struct IPhysMemObj {
	PyObject_HEAD
	IPhysicalMemory *object;
};

extern PyTypeObject IPhysMemType;
bool IPhysMemType_Init();
