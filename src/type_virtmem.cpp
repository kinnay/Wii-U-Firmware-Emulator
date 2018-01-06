
#include <Python.h>
#include "type_virtmem.h"
#include "type_ivirtmem.h"
#include "class_virtmem.h"
#include "errors.h"
#include "common.h"

void VirtMem_dealloc(VirtMemObj *self) {
	if (self->object) {
		delete self->object;
	}
	Py_TYPE(self)->tp_free((PyObject *)self);
}

PyObject *VirtMem_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
	VirtMemObj *self = (VirtMemObj *)IVirtMemType.tp_new(type, args, kwargs);
	if (self) {
		self->object = NULL;
	}
	return (PyObject *)self;
}

int VirtMem_init(VirtMemObj *self, PyObject *args, PyObject *kwargs) {
	CHECK_NOT_INIT(self->object);
	
	self->object = new VirtualMemory();
	if (!self->object) {
		PyErr_NoMemory();
		return -1;
	}
	self->super.object = self->object;
	return 0;
}

PyObject *VirtMem_addRange(VirtMemObj *self, PyObject *args) {
	CHECK_INIT(self->object);
	
	uint32_t virt, phys;
	uint64_t end; //May be 0x100000000
	if (!PyArg_ParseTuple(args, "ILI", &virt, &end, &phys)) return NULL;
	
	if (end > 0x100000000 || virt >= end) {
		ValueError("Invalid memory range");
		return NULL;
	}
	
	if (!self->object->addRange(virt, (uint32_t)end - 1, phys)) return NULL;
	Py_RETURN_NONE;
}

PyMethodDef VirtMem_methods[] = {
	{"add_range", (PyCFunction)VirtMem_addRange, METH_VARARGS, NULL},
	{NULL}
};

PyTypeObject VirtMemType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pyemu.VirtualMemory",
	sizeof(VirtMemObj),
	0, //itemsize
	(destructor)VirtMem_dealloc, //dealloc
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
	VirtMem_methods, //methods
	0, //members
	0, //getset
	&IVirtMemType, //base
	0, //dict
	0, //descr_get
	0, //descr_set
	0, //dictoffset
	(initproc)VirtMem_init, //init
	0, //allocate
	VirtMem_new, //new
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

bool VirtMemType_Init() {
	return PyType_Ready(&VirtMemType) == 0;
}
