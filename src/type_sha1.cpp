
#include <Python.h>
#include "type_sha1.h"
#include "class_sha1.h"
#include "errors.h"
#include "common.h"

void SHA1_dealloc(SHA1Obj *self) {
	if (self->object) {
		delete self->object;
	}
	Py_TYPE(self)->tp_free((PyObject *)self);
}

PyObject *SHA1_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
	SHA1Obj *self = (SHA1Obj *)type->tp_alloc(type, 0);
	if (self) {
		self->object = NULL;
	}
	return (PyObject *)self;
}

int SHA1_init(SHA1Obj *self, PyObject *args, PyObject *kwargs) {
	CHECK_NOT_INIT(self->object);
	
	self->object = new SHA1();
	if (!self->object) {
		PyErr_NoMemory();
		return -1;
	}
	return 0;
}

PyObject *SHA1_getH0(SHA1Obj *self, void *closure) {
	CHECK_INIT(self->object);
	return PyLong_FromUnsignedLong(self->object->h0);
}

PyObject *SHA1_getH1(SHA1Obj *self, void *closure) {
	CHECK_INIT(self->object);
	return PyLong_FromUnsignedLong(self->object->h1);
}

PyObject *SHA1_getH2(SHA1Obj *self, void *closure) {
	CHECK_INIT(self->object);
	return PyLong_FromUnsignedLong(self->object->h2);
}

PyObject *SHA1_getH3(SHA1Obj *self, void *closure) {
	CHECK_INIT(self->object);
	return PyLong_FromUnsignedLong(self->object->h3);
}

PyObject *SHA1_getH4(SHA1Obj *self, void *closure) {
	CHECK_INIT(self->object);
	return PyLong_FromUnsignedLong(self->object->h4);
}

int SHA1_setH0(SHA1Obj *self, PyObject *value, void *closure) {
	CHECK_INIT(self->object);
	if (!value) {
		TypeError("Cannot delete attribute");
		return -1;
	}

	uint32_t h0 = PyLong_AsUnsignedLong(value);
	if (PyErr_Occurred()) return -1;

	self->object->h0 = h0;
	return 0;
}

int SHA1_setH1(SHA1Obj *self, PyObject *value, void *closure) {
	CHECK_INIT(self->object);
	if (!value) {
		TypeError("Cannot delete attribute");
		return -1;
	}

	uint32_t h1 = PyLong_AsUnsignedLong(value);
	if (PyErr_Occurred()) return -1;

	self->object->h1 = h1;
	return 0;
}

int SHA1_setH2(SHA1Obj *self, PyObject *value, void *closure) {
	CHECK_INIT(self->object);
	if (!value) {
		TypeError("Cannot delete attribute");
		return -1;
	}

	uint32_t h2 = PyLong_AsUnsignedLong(value);
	if (PyErr_Occurred()) return -1;

	self->object->h2 = h2;
	return 0;
}

int SHA1_setH3(SHA1Obj *self, PyObject *value, void *closure) {
	CHECK_INIT(self->object);
	if (!value) {
		TypeError("Cannot delete attribute");
		return -1;
	}

	uint32_t h3 = PyLong_AsUnsignedLong(value);
	if (PyErr_Occurred()) return -1;

	self->object->h3 = h3;
	return 0;
}

int SHA1_setH4(SHA1Obj *self, PyObject *value, void *closure) {
	CHECK_INIT(self->object);
	if (!value) {
		TypeError("Cannot delete attribute");
		return -1;
	}

	uint32_t h4 = PyLong_AsUnsignedLong(value);
	if (PyErr_Occurred()) return -1;

	self->object->h4 = h4;
	return 0;
}

PyGetSetDef SHA1_props[] = {
	{"h0", (getter)SHA1_getH0, (setter)SHA1_setH0, 0, 0},
	{"h1", (getter)SHA1_getH1, (setter)SHA1_setH1, 0, 0},
	{"h2", (getter)SHA1_getH2, (setter)SHA1_setH2, 0, 0},
	{"h3", (getter)SHA1_getH3, (setter)SHA1_setH3, 0, 0},
	{"h4", (getter)SHA1_getH4, (setter)SHA1_setH4, 0, 0},
	{NULL}
};

PyObject *SHA1_processBlock(SHA1Obj *self, PyObject *args) {
	CHECK_INIT(self->object);
	
	void *data;
	int length;
	if (!PyArg_ParseTuple(args, "y#", &data, &length)) return NULL;
	if (length != 0x40) {
		ValueError("Block must contain exactly 64 bytes");
		return NULL;
	}
	self->object->processBlock(data);
	Py_RETURN_NONE;
}

PyMethodDef SHA1_methods[] = {
	{"process_block", (PyCFunction)SHA1_processBlock, METH_VARARGS, NULL},
	{NULL}
};

PyTypeObject SHA1Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pyemu.SHA1",
	sizeof(SHA1Obj),
	0, //itemsize
	(destructor)SHA1_dealloc, //dealloc
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
	"SHA-1 hashing algorithm", //doc
	0, //traverse
	0, //clear
	0, //richcompare
	0, //weaklistoffset
	0, //iter
	0, //iternext
	SHA1_methods, //methods
	0, //members
	SHA1_props, //getset
	0, //base
	0, //dict
	0, //descr_get
	0, //descr_set
	0, //dictoffset
	(initproc)SHA1_init, //init
	0, //allocate
	SHA1_new, //new
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

bool SHA1Type_Init() {
	return PyType_Ready(&SHA1Type) == 0;
}
