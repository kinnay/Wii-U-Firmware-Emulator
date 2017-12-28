
#include <Python.h>
#include "type_ppclock.h"
#include "class_ppclock.h"
#include "errors.h"
#include "common.h"

void PPCLock_dealloc(PPCLockObj *self) {
	if (self->object) {
		delete self->object;
	}
	Py_TYPE(self)->tp_free((PyObject *)self);
}

PyObject *PPCLock_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
	PPCLockObj *self = (PPCLockObj *)type->tp_alloc(type, 0);
	if (self) {
		self->object = NULL;
	}
	return (PyObject *)self;
}

int PPCLock_init(PPCLockObj *self, PyObject *args, PyObject *kwargs) {
	CHECK_NOT_INIT(self->object);
	
	self->object = new PPCLockMgr();
	if (!self->object) {
		PyErr_NoMemory();
		return -1;
	}
	return 0;
}

PyTypeObject PPCLockType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pyemu.PPCLockMgr",
	sizeof(PPCLockObj),
	0, //itemsize
	(destructor)PPCLock_dealloc, //dealloc
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
	"Reservation for lwarx/stwcx", //doc
	0, //traverse
	0, //clear
	0, //richcompare
	0, //weaklistoffset
	0, //iter
	0, //iternext
	0, //methods
	0, //members
	0, //getset
	0, //base
	0, //dict
	0, //descr_get
	0, //descr_set
	0, //dictoffset
	(initproc)PPCLock_init, //init
	0, //allocate
	PPCLock_new, //new
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

bool PPCLockType_Init() {
	return PyType_Ready(&PPCLockType) == 0;
}
