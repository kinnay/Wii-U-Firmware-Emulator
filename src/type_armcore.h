
#pragma once

#include <Python.h>
#include "class_armcore.h"

struct ARMCoreObj {
	PyObject_HEAD
	ARMCore *object;
};

extern PyTypeObject ARMCoreType;
bool ARMCoreType_Init();
