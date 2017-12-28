
#pragma once

#include <Python.h>
#include "class_elffile.h"

struct ELFProgramObj {
	PyObject_HEAD
	PyObject *parent;
	ELFProgram *object;
};

extern PyTypeObject ELFProgramType;
bool ELFProgramType_Init();
