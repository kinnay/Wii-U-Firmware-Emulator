
#pragma once

#include "type_ivirtmem.h"
#include "type_iphysmem.h"
#include "class_armmmu.h"

#include <Python.h>

struct ARMMMUObj {
	IVirtMemObj super;
	ARMMMU *object;
	IPhysMemObj *physmem;
};

extern PyTypeObject ARMMMUType;
bool ARMMMUType_Init();
