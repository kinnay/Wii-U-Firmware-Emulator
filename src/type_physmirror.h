
#pragma once

#include <Python.h>
#include <vector>
#include "type_iphysmem.h"
#include "type_ivirtmem.h"
#include "class_physmirror.h"

struct PhysMirrorObj {
	IPhysMemObj super;
	PhysicalMirror *object;
	IPhysMemObj *physmem;
	IVirtMemObj *translator;
};

extern PyTypeObject PhysMirrorType;
bool PhysMirrorType_Init();
