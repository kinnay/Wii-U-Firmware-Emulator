
#pragma once

#include <Python.h>
#include "class_interpreter.h"
#include "type_physmem.h"
#include "type_ivirtmem.h"

struct InterpreterObj {
	PyObject_HEAD
	Interpreter *object;
	PhysMemObj *physmem;
	IVirtMemObj *virtmem;
	PyObject *dataErrorCB;
	PyObject *fetchErrorCB;
	PyObject *breakpointCB;
	PyObject *watchpointReadCB;
	PyObject *watchpointWriteCB;
	PyObject *alarmCB;
};

extern PyTypeObject InterpreterType;
bool InterpreterType_Init();
