
#include <Python.h>
#include "type_interpreter.h"
#include "errors.h"
#include "common.h"

int Interpreter_clear(InterpreterObj *self) {
	Py_CLEAR(self->physmem);
	Py_CLEAR(self->virtmem);
	Py_CLEAR(self->dataErrorCB);
	Py_CLEAR(self->fetchErrorCB);
	Py_CLEAR(self->breakpointCB);
	Py_CLEAR(self->watchpointReadCB);
	Py_CLEAR(self->watchpointWriteCB);
	Py_CLEAR(self->alarmCB);
	return 0;
}

int Interpreter_traverse(InterpreterObj *self, visitproc visit, void *arg) {
	Py_VISIT(self->physmem);
	Py_VISIT(self->virtmem);
	Py_VISIT(self->dataErrorCB);
	Py_VISIT(self->fetchErrorCB);
	Py_VISIT(self->breakpointCB);
	Py_VISIT(self->watchpointReadCB);
	Py_VISIT(self->watchpointWriteCB);
	Py_VISIT(self->alarmCB);
	return 0;
}

PyObject *Interpreter_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
	if (type == &InterpreterType) {
		RuntimeError("Can't create instance of pyemu.Interpreter");
		return NULL;
	}

	InterpreterObj *self = (InterpreterObj *)type->tp_alloc(type, 0);
	if (self) {
		self->object = NULL;
		self->physmem = NULL;
		self->virtmem = NULL;
		self->dataErrorCB = NULL;
		self->fetchErrorCB = NULL;
		self->breakpointCB = NULL;
		self->watchpointReadCB = NULL;
		self->watchpointWriteCB = NULL;
		self->alarmCB = NULL;
	}
	return (PyObject *)self;
}

PyObject *Interpreter_run(InterpreterObj *self, PyObject *args) {
	CHECK_INIT(self->object);
	
	int steps = 0;
	if (!PyArg_ParseTuple(args, "|i", &steps)) return NULL;
	
	if (!self->object->run(steps)) {
		return NULL;
	}
	Py_RETURN_NONE;
}

PyObject *Interpreter_step(InterpreterObj *self, PyObject *args) {
	CHECK_INIT(self->object);
	if (!self->object->step()) {
		return NULL;
	}
	Py_RETURN_NONE;
}

PyObject *Interpreter_onDataError(InterpreterObj *self, PyObject *arg) {
	CHECK_INIT(self->object);
	if (!PyCallable_Check(arg)) {
		TypeError("Parameter must be callable");
		return NULL;
	}
	
	Py_INCREF(arg);
	Py_XDECREF(self->dataErrorCB);
	self->dataErrorCB = arg;
	
	self->object->setDataErrorCB(
		[arg] (uint32_t addr, bool isWrite) -> bool {
			PyErr_Clear();

			PyObject *args = Py_BuildValue("(IO)", addr, isWrite ? Py_True : Py_False);
			if (!args) return false;
			
			PyObject *result = PyObject_CallObject(arg, args);
			Py_DECREF(args);
			
			if (!result) {
				return false;
			}
			Py_DECREF(result);
			return true;
		}
	);
	
	Py_RETURN_NONE;
}

PyObject *Interpreter_onFetchError(InterpreterObj *self, PyObject *arg) {
	CHECK_INIT(self->object);
	if (!PyCallable_Check(arg)) {
		TypeError("Parameter must be callable");
		return NULL;
	}
	
	Py_INCREF(arg);
	Py_XDECREF(self->fetchErrorCB);
	self->fetchErrorCB = arg;
	
	self->object->setFetchErrorCB(
		[arg] (uint32_t addr) -> bool {
			PyErr_Clear();

			PyObject *args = Py_BuildValue("(I)", addr);
			if (!args) return false;
			
			PyObject *result = PyObject_CallObject(arg, args);
			Py_DECREF(args);
			
			if (!result) {
				return false;
			}
			Py_DECREF(result);
			return true;
		}
	);
	
	Py_RETURN_NONE;
}

PyObject *Interpreter_onBreakpoint(InterpreterObj *self, PyObject *arg) {
	CHECK_INIT(self->object);
	if (!PyCallable_Check(arg)) {
		TypeError("Parameter must be callable");
		return NULL;
	}
	
	Py_INCREF(arg);
	Py_XDECREF(self->breakpointCB);
	self->breakpointCB = arg;
	
	self->object->setBreakpointCB(
		[arg] (uint32_t addr) -> bool {
			PyObject *args = Py_BuildValue("(I)", addr);
			if (!args) return false;
			
			PyObject *result = PyObject_CallObject(arg, args);
			Py_DECREF(args);
			
			if (!result) {
				return false;
			}
			Py_DECREF(result);
			return true;
		}
	);
	
	Py_RETURN_NONE;
}

PyObject *Interpreter_addBreakpoint(InterpreterObj *self, PyObject *arg) {
	CHECK_INIT(self->object);
	
	uint32_t addr = PyLong_AsUnsignedLong(arg);
	if (PyErr_Occurred()) return NULL;
	
	self->object->breakpoints.push_back(addr);
	Py_RETURN_NONE;
}

PyObject *Interpreter_removeBreakpoint(InterpreterObj *self, PyObject *arg) {
	CHECK_INIT(self->object);
	
	uint32_t addr = PyLong_AsUnsignedLong(arg);
	if (PyErr_Occurred()) return NULL;
	
	auto &vec = self->object->breakpoints;
	auto position = std::find(vec.begin(), vec.end(), addr);
	if (position == vec.end()) {
		KeyError("0x%x", addr);
		return NULL;
	}
	vec.erase(position);
	Py_RETURN_NONE;
}

PyObject *Interpreter_onWatchpoint(InterpreterObj *self, PyObject *args) {
	CHECK_INIT(self->object);
	
	int write;
	PyObject *callback;
	if (!PyArg_ParseTuple(args, "pO", &write, &callback)) return NULL;
	
	Py_INCREF(callback);
	if (write) {
		Py_XDECREF(self->watchpointWriteCB);
		self->watchpointWriteCB = callback;
	}
	else {
		Py_XDECREF(self->watchpointReadCB);
		self->watchpointReadCB = callback;
	}
	
	self->object->setWatchpointCB(
		write != 0,
		[callback] (uint32_t addr) -> bool {
			PyObject *args = Py_BuildValue("(I)", addr);
			if (!args) return false;
			
			PyObject *result = PyObject_CallObject(callback, args);
			Py_DECREF(args);
			
			if (!result) {
				return false;
			}
			Py_DECREF(result);
			return true;
		}
	);
	
	Py_RETURN_NONE;
}

PyObject *Interpreter_addWatchpoint(InterpreterObj *self, PyObject *args) {
	CHECK_INIT(self->object);

	int write;
	uint32_t addr;
	if (!PyArg_ParseTuple(args, "pI", &write, &addr)) return NULL;
	
	if (write) {
		self->object->watchpointsWrite.push_back(addr);
	}
	else {
		self->object->watchpointsRead.push_back(addr);
	}
	Py_RETURN_NONE;
}

PyObject *Interpreter_setAlarm(InterpreterObj *self, PyObject *args) {
	CHECK_INIT(self->object);
	
	uint32_t alarm;
	PyObject *callback;
	if (!PyArg_ParseTuple(args, "IO", &alarm, &callback)) return NULL;
	
	if (!PyCallable_Check(callback)) {
		TypeError("Parameter must be callable");
		return NULL;
	}
	
	Py_INCREF(callback);
	Py_XDECREF(self->alarmCB);
	self->alarmCB = callback;
	
	self->object->setAlarm(
		alarm,
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

PyMethodDef Interpreter_methods[] = {
	{"run", (PyCFunction)Interpreter_run, METH_VARARGS, NULL},
	{"step", (PyCFunction)Interpreter_step, METH_NOARGS, NULL},
	{"on_data_error", (PyCFunction)Interpreter_onDataError, METH_O, NULL},
	{"on_fetch_error", (PyCFunction)Interpreter_onFetchError, METH_O, NULL},
	{"on_breakpoint", (PyCFunction)Interpreter_onBreakpoint, METH_O, NULL},
	{"add_breakpoint", (PyCFunction)Interpreter_addBreakpoint, METH_O, NULL},
	{"remove_breakpoint", (PyCFunction)Interpreter_removeBreakpoint, METH_O, NULL},
	{"on_watchpoint", (PyCFunction)Interpreter_onWatchpoint, METH_VARARGS, NULL},
	{"add_watchpoint", (PyCFunction)Interpreter_addWatchpoint, METH_VARARGS, NULL},
	{"set_alarm", (PyCFunction)Interpreter_setAlarm, METH_VARARGS, NULL},
	{NULL}
};

PyTypeObject InterpreterType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pyemu.Interpreter", 
	sizeof(InterpreterObj),
	0, //itemsize
	0, //dealloc
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
	"An interpreter", //doc
	(traverseproc)Interpreter_traverse, //traverse
	(inquiry)Interpreter_clear, //clear
	0, //richcompare
	0, //weaklistoffset
	0, //iter
	0, //iternext
	Interpreter_methods, //methods
	0, //members
	0, //getset
	0, //base
	0, //dict
	0, //descr_get
	0, //descr_set
	0, //dictoffset
	0, //init
	0, //allocate
	Interpreter_new, //new
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

bool InterpreterType_Init() {
	return PyType_Ready(&InterpreterType) == 0;
}
