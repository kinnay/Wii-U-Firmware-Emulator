
#pragma once

#include <Python.h>
#include <vector>
#include "type_iphysmem.h"
#include "class_physmem.h"

struct PhysMemObj {
	IPhysMemObj super;
	PhysicalMemory *object;
	std::vector<PyObject *> callbacks;
};

extern PyTypeObject PhysMemType;
bool PhysMemType_Init();
