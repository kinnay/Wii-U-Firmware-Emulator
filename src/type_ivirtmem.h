
#pragma once

#include <Python.h>
#include "interface_virtmem.h"

struct IVirtMemObj {
	PyObject_HEAD
	IVirtualMemory *object;
};

extern PyTypeObject IVirtMemType;
bool IVirtMemType_Init();
