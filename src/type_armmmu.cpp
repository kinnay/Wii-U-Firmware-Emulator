
#include <Python.h>
#include "type_ivirtmem.h"
#include "type_iphysmem.h"
#include "type_armmmu.h"
#include "class_armmmu.h"
#include "errors.h"
#include "common.h"

int ARMMMU_clear(ARMMMUObj *self) {
	Py_CLEAR(self->physmem);
	return 0;
}

int ARMMMU_traverse(ARMMMUObj *self, visitproc visit, void *arg) {
	Py_VISIT(self->physmem);
	return 0;
}

void ARMMMU_dealloc(ARMMMUObj *self) {
	if (self->object) {
		delete self->object;
	}
	PyObject_GC_UnTrack(self);
	ARMMMU_clear(self);
	Py_TYPE(self)->tp_free((PyObject *)self);
}

PyObject *ARMMMU_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
	ARMMMUObj *self = (ARMMMUObj *)IVirtMemType.tp_new(type, args, kwargs);
	if (self) {
		self->object = NULL;
	}
	return (PyObject *)self;
}

int ARMMMU_init(ARMMMUObj *self, PyObject *args, PyObject *kwargs) {
	CHECK_NOT_INIT(self->object);
	
	IPhysMemObj *physmem;
	bool bigEndian;
	if (!PyArg_ParseTuple(args, "O!p", &IPhysMemType, &physmem, &bigEndian)) {
		return -1;
	}
	
	if (!physmem->object) {
		RuntimeError("Physical memory object must be initialized");
		return -1;
	}
	
	self->object = new ARMMMU(physmem->object, bigEndian);
	if (!self->object) {
		PyErr_NoMemory();
		return -1;
	}
	self->super.object = self->object;
	
	Py_INCREF(physmem);
	self->physmem = physmem;
	return 0;
}

PyObject *ARMMMU_setTranslationTableBase(ARMMMUObj *self, PyObject *arg) {
	CHECK_INIT(self->object);
	
	uint32_t base = PyLong_AsUnsignedLong(arg);
	if (PyErr_Occurred()) return NULL;
	
	self->object->setTranslationTableBase(base);
	Py_RETURN_NONE;
}

PyMethodDef ARMMMU_methods[] = {
	{"set_translation_table_base", (PyCFunction)ARMMMU_setTranslationTableBase, METH_O, NULL},
	{NULL}
};

PyTypeObject ARMMMUType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pyemu.ARMMMU",
	sizeof(ARMMMUObj),
	0, //itemsize
	(destructor)ARMMMU_dealloc, //dealloc
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
	"Represents virtual memory", //doc
	(traverseproc)ARMMMU_traverse, //traverse
	(inquiry)ARMMMU_clear, //clear
	0, //richcompare
	0, //weaklistoffset
	0, //iter
	0, //iternext
	ARMMMU_methods, //methods
	0, //members
	0, //getset
	&IVirtMemType, //base
	0, //dict
	0, //descr_get
	0, //descr_set
	0, //dictoffset
	(initproc)ARMMMU_init, //init
	0, //allocate
	ARMMMU_new, //new
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

bool ARMMMUType_Init() {
	return PyType_Ready(&ARMMMUType) == 0;
}
