
#pragma once

#include <Python.h>
#include "type_interpreter.h"
#include "type_armcore.h"
#include "class_arminterp.h"

struct ARMInterpObj {
	InterpreterObj super;
	ARMInterpreter *object;
	ARMCoreObj *core;
	PyObject *coprocReadCB;
	PyObject *coprocWriteCB;
	PyObject *softwareInterruptCB;
	PyObject *undefinedCB;
};

extern PyTypeObject ARMInterpType;
bool ARMInterpType_Init();
