
#pragma once

#include <Python.h>
#include "type_ivirtmem.h"
#include "class_virtmem.h"

struct VirtMemObj {
	IVirtMemObj super;
	VirtualMemory *object;
};

extern PyTypeObject VirtMemType;
bool VirtMemType_Init();
