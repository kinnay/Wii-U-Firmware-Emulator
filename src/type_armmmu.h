
#pragma once

#include "type_ivirtmem.h"
#include "type_physmem.h"
#include "class_armmmu.h"

#include <Python.h>

struct ARMMMUObj {
	IVirtMemObj super;
	ARMMMU *object;
	PhysMemObj *physmem;
};

extern PyTypeObject ARMMMUType;
bool ARMMMUType_Init();
