
#include <Python.h>
#include "type_armcore.h"
#include "class_armcore.h"
#include "errors.h"
#include "common.h"

void ARMCore_dealloc(ARMCoreObj *self) {
	if (self->object) {
		delete self->object;
	}
	Py_TYPE(self)->tp_free((PyObject *)self);
}

PyObject *ARMCore_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
	ARMCoreObj *self = (ARMCoreObj *)type->tp_alloc(type, 0);
	if (self) {
		self->object = NULL;
	}
	return (PyObject *)self;
}

int ARMCore_init(ARMCoreObj *self, PyObject *args, PyObject *kwargs) {
	CHECK_NOT_INIT(self->object);
	
	self->object = new ARMCore();
	if (!self->object) {
		PyErr_NoMemory();
		return -1;
	}
	return 0;
}

PyObject *ARMCore_getReg(ARMCoreObj *self, PyObject *args) {
	CHECK_INIT(self->object);
	
	int reg;
	if (!PyArg_ParseTuple(args, "i", &reg)) return NULL;
	if (reg < 0 || reg > 15) {
		ValueError("Invalid register");
		return NULL;
	}
	
	return PyLong_FromUnsignedLong(self->object->regs[reg]);
}

PyObject *ARMCore_setReg(ARMCoreObj *self, PyObject *args) {
	CHECK_INIT(self->object);
	
	int reg;
	uint32_t value;
	if (!PyArg_ParseTuple(args, "iI", &reg, &value)) return NULL;
	if (reg < 0 || reg > 15) {
		ValueError("Invalid register");
		return NULL;
	}
	
	self->object->regs[reg] = value;
	Py_RETURN_NONE;
}

PyObject *ARMCore_getCpsr(ARMCoreObj *self, PyObject *args) {
	CHECK_INIT(self->object);
	return PyLong_FromUnsignedLong(self->object->cpsr.value);
}

PyObject *ARMCore_isThumb(ARMCoreObj *self, PyObject *args) {
	CHECK_INIT(self->object);
	
	if (self->object->thumb) {
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

PyObject *ARMCore_setThumb(ARMCoreObj *self, PyObject *args) {
	CHECK_INIT(self->object);
	
	bool thumb;
	if (!PyArg_ParseTuple(args, "p", &thumb)) return NULL;
	
	self->object->setThumb(thumb);
	Py_RETURN_NONE;
}

PyObject *ARMCore_triggerException(ARMCoreObj *self, PyObject *arg) {
	CHECK_INIT(self->object);

	int type = PyLong_AsLong(arg);
	if (PyErr_Occurred()) return NULL;
	
	if (type != ARMCore::UndefinedInstruction && type != ARMCore::DataAbort &&
		type != ARMCore::InterruptRequest)
	{
		ValueError("Invalid exception type");
		return NULL;
	}
	
	self->object->triggerException((ARMCore::ExceptionType)type);
	Py_RETURN_NONE;
}

PyMethodDef ARMCore_methods[] = {
	{"reg", (PyCFunction)ARMCore_getReg, METH_VARARGS, NULL},
	{"setreg", (PyCFunction)ARMCore_setReg, METH_VARARGS, NULL},
	{"cpsr", (PyCFunction)ARMCore_getCpsr, METH_NOARGS, NULL},
	{"is_thumb", (PyCFunction)ARMCore_isThumb, METH_NOARGS, NULL},
	{"set_thumb", (PyCFunction)ARMCore_setThumb, METH_VARARGS, NULL},
	{"trigger_exception", (PyCFunction)ARMCore_triggerException, METH_O, NULL},
	{NULL}
};

PyTypeObject ARMCoreType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pyemu.ARMCore",
	sizeof(ARMCoreObj),
	0, //itemsize
	(destructor)ARMCore_dealloc, //dealloc
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
	Py_TPFLAGS_DEFAULT,
	"Represents an ARM core", //doc
	0, //traverse
	0, //clear
	0, //richcompare
	0, //weaklistoffset
	0, //iter
	0, //iternext
	ARMCore_methods, //methods
	0, //members
	0, //getset
	0, //base
	0, //dict
	0, //descr_get
	0, //descr_set
	0, //dictoffset
	(initproc)ARMCore_init, //init
	0, //allocate
	ARMCore_new, //new
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

bool ARMCoreType_Init() {
	PyObject *dict = PyDict_New();
	
	PyDict_SetItemString(dict, "R0", PyLong_FromLong(ARMCore::R0));
	PyDict_SetItemString(dict, "R1", PyLong_FromLong(ARMCore::R1));
	PyDict_SetItemString(dict, "R2", PyLong_FromLong(ARMCore::R2));
	PyDict_SetItemString(dict, "R3", PyLong_FromLong(ARMCore::R3));
	PyDict_SetItemString(dict, "R4", PyLong_FromLong(ARMCore::R4));
	PyDict_SetItemString(dict, "R5", PyLong_FromLong(ARMCore::R5));
	PyDict_SetItemString(dict, "R6", PyLong_FromLong(ARMCore::R6));
	PyDict_SetItemString(dict, "R7", PyLong_FromLong(ARMCore::R7));
	PyDict_SetItemString(dict, "R8", PyLong_FromLong(ARMCore::R8));
	PyDict_SetItemString(dict, "R9", PyLong_FromLong(ARMCore::R9));
	PyDict_SetItemString(dict, "R10", PyLong_FromLong(ARMCore::R10));
	PyDict_SetItemString(dict, "R11", PyLong_FromLong(ARMCore::R11));
	PyDict_SetItemString(dict, "R12", PyLong_FromLong(ARMCore::R12));
	PyDict_SetItemString(dict, "SP", PyLong_FromLong(ARMCore::SP));
	PyDict_SetItemString(dict, "LR", PyLong_FromLong(ARMCore::LR));
	PyDict_SetItemString(dict, "PC", PyLong_FromLong(ARMCore::PC));
	
	PyDict_SetItemString(dict, "N", PyLong_FromLong(ARMCore::N));
	PyDict_SetItemString(dict, "Z", PyLong_FromLong(ARMCore::Z));
	PyDict_SetItemString(dict, "C", PyLong_FromLong(ARMCore::C));
	PyDict_SetItemString(dict, "V", PyLong_FromLong(ARMCore::V));
	
	PyDict_SetItemString(dict, "UNDEFINED_INSTRUCTION", PyLong_FromLong(ARMCore::UndefinedInstruction));
	PyDict_SetItemString(dict, "DATA_ABORT", PyLong_FromLong(ARMCore::DataAbort));
	PyDict_SetItemString(dict, "IRQ", PyLong_FromLong(ARMCore::InterruptRequest));
	
	ARMCoreType.tp_dict = dict;
	return PyType_Ready(&ARMCoreType) == 0;
}
