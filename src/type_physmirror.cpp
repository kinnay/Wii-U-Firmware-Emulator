
#include <Python.h>
#include "type_iphysmem.h"
#include "type_ivirtmem.h"
#include "type_physmirror.h"
#include "class_physmirror.h"
#include "errors.h"
#include "common.h"

int PhysMirror_clear(PhysMirrorObj *self) {
	Py_CLEAR(self->physmem);
	Py_CLEAR(self->translator);
	return 0;
}

int PhysMirror_traverse(PhysMirrorObj *self, visitproc visit, void *arg) {
	Py_VISIT(self->physmem);
	Py_VISIT(self->translator);
	return 0;
}

void PhysMirror_dealloc(PhysMirrorObj *self) {
	if (self->object) {
		delete self->object;
	}
	PyObject_GC_UnTrack(self);
	PhysMirror_clear(self);
	Py_TYPE(self)->tp_free((PyObject *)self);
}

PyObject *PhysMirror_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
	PhysMirrorObj *self = (PhysMirrorObj *)IPhysMemType.tp_new(type, args, kwargs);
	if (self) {
		self->object = NULL;
	}
	return (PyObject *)self;
}

int PhysMirror_init(PhysMirrorObj *self, PyObject *args, PyObject *kwargs) {
	CHECK_NOT_INIT(self->object);
	
	IPhysMemObj *physmem;
	IVirtMemObj *translator;
	if (!PyArg_ParseTuple(args, "O!O!", &IPhysMemType, &physmem, &IVirtMemType, &translator)) {
		return -1;
	}
	
	if (!physmem->object) {
		RuntimeError("Physical memory object must be initialized");
		return -1;
	}
	
	if (!translator->object) {
		RuntimeError("Virtual memory object must be initialized");
		return -1;
	}
	
	self->object = new PhysicalMirror(physmem->object, translator->object);
	if (!self->object) {
		PyErr_NoMemory();
		return -1;
	}
	self->super.object = self->object;
	
	Py_INCREF(physmem);
	Py_INCREF(translator);
	self->physmem = physmem;
	self->translator = translator;
	return 0;
}

PyTypeObject PhysMirrorType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pyemu.PhysicalMirror", 
	sizeof(PhysMirrorObj),
	0, //itemsize
	(destructor)PhysMirror_dealloc, //dealloc
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
	"Represents physical memory", //doc
	(traverseproc)PhysMirror_traverse, //traverse
	(inquiry)PhysMirror_clear, //clear
	0, //richcompare
	0, //weaklistoffset
	0, //iter
	0, //iternext
	0, //methods
	0, //members
	0, //getset
	&IPhysMemType, //base
	0, //dict
	0, //descr_get
	0, //descr_set
	0, //dictoffset
	(initproc)PhysMirror_init, //init
	0, //allocate
	PhysMirror_new, //new
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

bool PhysMirrorType_Init() {
	return PyType_Ready(&PhysMirrorType) == 0;
}
