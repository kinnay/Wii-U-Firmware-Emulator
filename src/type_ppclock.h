
#pragma once

#include <Python.h>
#include "class_ppclock.h"

struct PPCLockObj {
	PyObject_HEAD
	PPCLockMgr *object;
};

extern PyTypeObject PPCLockType;
bool PPCLockType_Init();
