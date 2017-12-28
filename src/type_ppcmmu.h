
#pragma once

#include "type_ivirtmem.h"
#include "type_iphysmem.h"
#include "class_ppcmmu.h"

#include <Python.h>

struct PPCMMUObj {
	IVirtMemObj super;
	PPCMMU *object;
	IPhysMemObj *physmem;
};

extern PyTypeObject PPCMMUType;
bool PPCMMUType_Init();
