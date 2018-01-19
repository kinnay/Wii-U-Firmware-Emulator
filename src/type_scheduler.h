
#pragma once

#include <Python.h>
#include "class_scheduler.h"
#include <vector>

struct SchedulerObj {
	PyObject_HEAD
	Scheduler *object;
	std::vector<PyObject *> alarmCBs;
};

extern PyTypeObject SchedulerType;
bool SchedulerType_Init();
