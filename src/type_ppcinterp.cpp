
#include <Python.h>
#include <algorithm>
#include "type_interpreter.h"
#include "type_ppcinterp.h"
#include "type_ppccore.h"
#include "type_physmem.h"
#include "type_ivirtmem.h"
#include "class_ppcinterp.h"
#include "errors.h"
#include "common.h"

int PPCInterp_clear(PPCInterpObj *self) {
	InterpreterType.tp_clear((PyObject *)self);
	Py_CLEAR(self->core);
	return 0;
}

int PPCInterp_traverse(PPCInterpObj *self, visitproc visit, void *arg) {
	InterpreterType.tp_traverse((PyObject *)self, visit, arg);
	Py_VISIT(self->core);
	return 0;
}

void PPCInterp_dealloc(PPCInterpObj *self) {
	if (self->object) {
		delete self->object;
	}
	PyObject_GC_UnTrack(self);
	PPCInterp_clear(self);
	Py_TYPE(self)->tp_free((PyObject *)self);
}

PyObject *PPCInterp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
	PPCInterpObj *self = (PPCInterpObj *)InterpreterType.tp_new(type, args, kwargs);
	if (self) {
		self->object = NULL;
	}
	return (PyObject *)self;
}

int PPCInterp_init(PPCInterpObj *self, PyObject *args, PyObject *kwargs) {
	CHECK_NOT_INIT(self->object);
	
	PPCCoreObj *core;
	PhysMemObj *physmem;
	IVirtMemObj *virtmem;
	if (!PyArg_ParseTuple(args, "O!O!O!", &PPCCoreType, &core,
		&PhysMemType, &physmem, &IVirtMemType, &virtmem))
	{
		return -1;
	}
	
	if (!core->object) {
		RuntimeError("Core object must be initialized");
		return -1;
	}
	
	if (!physmem->object) {
		RuntimeError("Physical memory object must be initialized");
		return -1;
	}
	
	if (!virtmem->object) {
		RuntimeError("Virtual memory object must be initialized");
	}
	
	self->object = new PPCInterpreter(core->object, physmem->object, virtmem->object);
	if (!self->object) {
		PyErr_NoMemory();
		return -1;
	}
	self->super.object = self->object;
	
	Py_INCREF(core);
	Py_INCREF(physmem);
	Py_INCREF(virtmem);
	self->core = core;
	self->super.physmem = physmem;
	self->super.virtmem = virtmem;
	return 0;
}

PyTypeObject PPCInterpType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pyemu.PPCInterpreter", 
	sizeof(PPCInterpObj),
	0, //itemsize
	(destructor)PPCInterp_dealloc, //dealloc
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
	"A PowerPC interpreter", //doc
	(traverseproc)PPCInterp_traverse, //traverse
	(inquiry)PPCInterp_clear, //clear
	0, //richcompare
	0, //weaklistoffset
	0, //iter
	0, //iternext
	0, //methods
	0, //members
	0, //getset
	&InterpreterType, //base
	0, //dict
	0, //descr_get
	0, //descr_set
	0, //dictoffset
	(initproc)PPCInterp_init, //init
	0, //allocate
	PPCInterp_new, //new
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

bool PPCInterpType_Init() {
	return PyType_Ready(&PPCInterpType) == 0;
}
