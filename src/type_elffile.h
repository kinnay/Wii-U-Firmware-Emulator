
#pragma once

#include <Python.h>
#include "class_elffile.h"

struct ELFFileObj {
	PyObject_HEAD
	ELFFile *object;
};

extern PyTypeObject ELFFileType;
bool ELFFileType_Init();
