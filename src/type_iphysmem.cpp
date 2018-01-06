
#include <Python.h>
#include "type_iphysmem.h"
#include "common.h"
#include "errors.h"

PyObject *IPhysMem_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
	if (type == &IPhysMemType) {
		RuntimeError("Can't create instance of pyemu.IPhysicalMemory");
		return NULL;
	}
	
	IPhysMemObj *self = (IPhysMemObj *)type->tp_alloc(type, 0);
	if (self) {
		self->object = NULL;
	}
	return (PyObject *)self;
}

PyObject *IPhysMem_read(IPhysMemObj *self, PyObject *args) {
	CHECK_INIT(self->object);
	
	uint32_t addr;
	int length;
	if (!PyArg_ParseTuple(args, "Ii", &addr, &length)) return NULL;
	
	if (!length) {
		return PyBytes_FromStringAndSize(NULL, 0);
	}
	
	char *buffer = new char[length];
	if (!buffer) {
		PyErr_NoMemory();
		return NULL;
	}
	
	if (self->object->read(addr, buffer, length) < 0) {
		delete buffer;
		return NULL;
	}
	
	PyObject *result = PyBytes_FromStringAndSize(buffer, length);
	delete buffer;
	
	return result;
}

PyObject *IPhysMem_write(IPhysMemObj *self, PyObject *args) {
	CHECK_INIT(self->object);
	
	uint32_t addr;
	void *data;
	int length;
	if (!PyArg_ParseTuple(args, "Iy#", &addr, &data, &length)) return NULL;
	if (length) {
		if (self->object->write(addr, data, length) < 0) return NULL;
	}
	Py_RETURN_NONE;
}

PyMethodDef IPhysMem_methods[] = {
	{"read", (PyCFunction)IPhysMem_read, METH_VARARGS, NULL},
	{"write", (PyCFunction)IPhysMem_write, METH_VARARGS, NULL},
	{NULL}
};

PyTypeObject IPhysMemType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pyemu.IPhysicalMemory",
	sizeof(IPhysMemObj),
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
	"Represents physical memory", //doc
	0, //traverse
	0, //clear
	0, //richcompare
	0, //weaklistoffset
	0, //iter
	0, //iternext
	IPhysMem_methods, //methods
	0, //members
	0, //getset
	0, //base
	0, //dict
	0, //descr_get
	0, //descr_set
	0, //dictoffset
	0, //init
	0, //allocate
	IPhysMem_new, //new
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

bool IPhysMemType_Init() {
	return PyType_Ready(&IPhysMemType) == 0;
}
