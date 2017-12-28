
#pragma once

#include <Python.h>
#include "class_ppcinterp.h"
#include "type_ppccore.h"
#include "type_interpreter.h"

struct PPCInterpObj {
	InterpreterObj super;
	PPCInterpreter *object;
	PPCCoreObj *core;
};

extern PyTypeObject PPCInterpType;
bool PPCInterpType_Init();
