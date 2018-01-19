
#pragma once

#include <Python.h>
#include "type_ppclock.h"
#include "class_ppccore.h"

struct PPCCoreObj {
	PyObject_HEAD
	PPCCore *object;
	PPCLockObj *lockMgr;
	PyObject *sprReadCB;
	PyObject *sprWriteCB;
	PyObject *srReadCB;
	PyObject *srWriteCB;
};

extern PyTypeObject PPCCoreType;
bool PPCCoreType_Init();
