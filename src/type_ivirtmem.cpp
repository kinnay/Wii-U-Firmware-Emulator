
#include <Python.h>
#include "interface_virtmem.h"
#include "type_ivirtmem.h"
#include "errors.h"
#include "common.h"

PyObject *IVirtMem_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
	if (type == &IVirtMemType) {
		RuntimeError("Can't create instance of pyemu.IVirtualMemory");
		return NULL;
	}
	
	IVirtMemObj *self = (IVirtMemObj *)type->tp_alloc(type, 0);
	if (self) {
		self->object = NULL;
	}
	return (PyObject *)self;
}

PyObject *IVirtMem_translate(IVirtMemObj *self, PyObject *args) {
	CHECK_INIT(self->object);
	
	uint32_t addr;
	int type = IVirtualMemory::DataRead;
	if (!PyArg_ParseTuple(args, "I|i", &addr, &type)) return NULL;
	if (type < 0 || type > 2) {
		ValueError("Invalid access type");
		return NULL;
	}
	
	if (!self->object->translate(&addr, 1, (IVirtualMemory::Access)type)) return NULL;
	return PyLong_FromUnsignedLong(addr);
}

PyObject *IVirtMem_setCacheEnabled(IVirtMemObj *self, PyObject *arg) {
	CHECK_INIT(self->object);
	self->object->setCacheEnabled(PyObject_IsTrue(arg) != 0);
	Py_RETURN_NONE;
}

PyObject *IVirtMem_invalidateCache(IVirtMemObj *self, PyObject *arg) {
	CHECK_INIT(self->object);
	self->object->invalidateCache();
	Py_RETURN_NONE;
}

PyMethodDef IVirtMem_methods[] = {
	{"translate", (PyCFunction)IVirtMem_translate, METH_VARARGS, NULL},
	{"set_cache_enabled", (PyCFunction)IVirtMem_setCacheEnabled, METH_O, NULL},
	{"invalidate_cache", (PyCFunction)IVirtMem_invalidateCache, METH_NOARGS, NULL},
	{NULL}
};

PyTypeObject IVirtMemType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pyemu.IVirtualMemory",
	sizeof(IVirtMemObj),
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
	Py_TPFLAGS_DEFAULT,
	"Represents virtual memory", //doc
	0, //traverse
	0, //clear
	0, //richcompare
	0, //weaklistoffset
	0, //iter
	0, //iternext
	IVirtMem_methods, //methods
	0, //members
	0, //getset
	0, //base
	0, //dict
	0, //descr_get
	0, //descr_set
	0, //dictoffset
	0, //init
	0, //allocate
	IVirtMem_new, //new
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

bool IVirtMemType_Init() {
	PyObject *dict = PyDict_New();
	
	PyDict_SetItemString(dict, "DATA_READ", PyLong_FromLong(IVirtualMemory::DataRead));
	PyDict_SetItemString(dict, "DATA_WRITE", PyLong_FromLong(IVirtualMemory::DataWrite));
	PyDict_SetItemString(dict, "INSTRUCTION", PyLong_FromLong(IVirtualMemory::Instruction));
	
	IVirtMemType.tp_dict = dict;
	return PyType_Ready(&IVirtMemType) == 0;
}
