
#pragma once

#include <Python.h>
#include <vector>
#include "class_physmem.h"

struct PhysMemObj {
	PyObject_HEAD
	PhysicalMemory *object;
	std::vector<PyObject *> callbacks;
};

extern PyTypeObject PhysMemType;
bool PhysMemType_Init();
