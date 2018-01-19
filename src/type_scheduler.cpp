
#include <Python.h>
#include "type_scheduler.h"
#include "type_interpreter.h"
#include "class_scheduler.h"
#include "errors.h"
#include "common.h"

int Scheduler_clear(SchedulerObj *self) {
	for (PyObject *alarmCB : self->alarmCBs) {
		Py_CLEAR(alarmCB);
	}
	return 0;
}

int Scheduler_traverse(SchedulerObj *self, visitproc visit, void *arg) {
	for (PyObject *alarmCB : self->alarmCBs) {
		Py_VISIT(alarmCB);
	}
	return 0;
}

void Scheduler_dealloc(SchedulerObj *self) {
	if (self->object) {
		delete self->object;
	}
	Py_TYPE(self)->tp_free((PyObject *)self);
}

PyObject *Scheduler_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
	SchedulerObj *self = (SchedulerObj *)type->tp_alloc(type, 0);
	if (self) {
		self->object = NULL;
	}
	return (PyObject *)self;
}

int Scheduler_init(SchedulerObj *self, PyObject *args, PyObject *kwargs) {
	CHECK_NOT_INIT(self->object);
	
	self->object = new Scheduler();
	if (!self->object) {
		PyErr_NoMemory();
		return -1;
	}
	return 0;
}

PyObject *Scheduler_add(SchedulerObj *self, PyObject *args) {
	CHECK_INIT(self->object);
	
	InterpreterObj *interpreter;
	int steps;
	if (!PyArg_ParseTuple(args, "O!i", &InterpreterType, &interpreter, &steps)) return NULL;
	
	if (!interpreter->object) {
		RuntimeError("Interpreter object must be initialized");
		return NULL;
	}
	
	if (steps <= 0) {
		ValueError("Steps must be greater than zero");
		return NULL;
	}
	
	self->object->add(interpreter->object, steps);
	Py_RETURN_NONE;
}

PyObject *Scheduler_resume(SchedulerObj *self, PyObject *arg) {
	CHECK_INIT(self->object);
	
	uint32_t index = PyLong_AsUnsignedLong(arg);
	if (PyErr_Occurred()) return NULL;
	if (!self->object->resume(index)) return NULL;
	Py_RETURN_NONE;
}

PyObject *Scheduler_run(SchedulerObj *self, PyObject *args) {
	CHECK_INIT(self->object);
	if (!self->object->run()) return NULL;
	Py_RETURN_NONE;
}

PyObject *Scheduler_addAlarm(SchedulerObj *self, PyObject *args) {
	CHECK_INIT(self->object);

	uint32_t interval;
	PyObject *callback;
	if (!PyArg_ParseTuple(args, "IO", &interval, &callback)) return NULL;
	
	if (!PyCallable_Check(callback)) {
		TypeError("Parameter must be callable");
		return NULL;
	}
	
	Py_INCREF(callback);
	self->alarmCBs.push_back(callback);
	
	self->object->addAlarm(
		interval,
		[callback] () -> bool {
			PyObject *result = PyObject_CallObject(callback, NULL);
			if (!result) {
				return false;
			}
			Py_DECREF(result);
			return true;
		}
	);
	
	Py_RETURN_NONE;
}

PyMethodDef Scheduler_methods[] = {
	{"add", (PyCFunction)Scheduler_add, METH_VARARGS, NULL},
	{"resume", (PyCFunction)Scheduler_resume, METH_O, NULL},
	{"run", (PyCFunction)Scheduler_run, METH_NOARGS, NULL},
	{"add_alarm", (PyCFunction)Scheduler_addAlarm, METH_VARARGS, NULL},
	{NULL}
};

PyObject *Scheduler_getIndex(SchedulerObj *self, void *closure) {
	CHECK_INIT(self->object);
	return PyLong_FromUnsignedLong(self->object->currentIndex());
}

PyGetSetDef Scheduler_props[] = {
	{"index", (getter)Scheduler_getIndex, 0, 0, 0},
	{NULL}
};

PyTypeObject SchedulerType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pyemu.Scheduler",
	sizeof(SchedulerObj),
	0, //itemsize
	(destructor)Scheduler_dealloc, //dealloc
	0, //print
	0, //getattr
	0, //setattr
	0, //as_async
	0, //repr
	0, //as_number
	0, //as_sequence
	0, //as_mapping
	0, //hash
	0, //call
	0, //str
	0, //getattro
	0, //setattro
	0, //as_buffer
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
	"Scheduler for interpreters", //doc
	(traverseproc)Scheduler_traverse, //traverse
	(inquiry)Scheduler_clear, //clear
	0, //richcompare
	0, //weaklistoffset
	0, //iter
	0, //iternext
	Scheduler_methods, //methods
	0, //members
	Scheduler_props, //getset
	0, //base
	0, //dict
	0, //descr_get
	0, //descr_set
	0, //dictoffset
	(initproc)Scheduler_init, //init
	0, //allocate
	Scheduler_new, //new
	0, //free
	0, //is_gc
	0, //bases
	0, //mro
	0, //cache
	0, //subclasses
	0, //weaklist
	0, //del
	0, //version_tag
	0  //finalize
};

bool SchedulerType_Init() {
	return PyType_Ready(&SchedulerType) == 0;
}
