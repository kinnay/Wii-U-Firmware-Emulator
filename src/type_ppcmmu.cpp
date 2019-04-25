
#include <Python.h>
#include "type_ivirtmem.h"
#include "type_physmem.h"
#include "type_ppcmmu.h"
#include "class_ppcmmu.h"
#include "errors.h"
#include "common.h"

int PPCMMU_clear(PPCMMUObj *self) {
	Py_CLEAR(self->physmem);
	return 0;
}

int PPCMMU_traverse(PPCMMUObj *self, visitproc visit, void *arg) {
	Py_VISIT(self->physmem);
	return 0;
}

void PPCMMU_dealloc(PPCMMUObj *self) {
	if (self->object) {
		delete self->object;
	}
	PyObject_GC_UnTrack(self);
	PPCMMU_clear(self);
	Py_TYPE(self)->tp_free((PyObject *)self);
}

PyObject *PPCMMU_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
	PPCMMUObj *self = (PPCMMUObj *)IVirtMemType.tp_new(type, args, kwargs);
	if (self) {
		self->object = NULL;
	}
	return (PyObject *)self;
}

int PPCMMU_init(PPCMMUObj *self, PyObject *args, PyObject *kwargs) {
	CHECK_NOT_INIT(self->object);
	
	PhysMemObj *physmem;
	if (!PyArg_ParseTuple(args, "O!", &PhysMemType, &physmem)) {
		return -1;
	}
	
	if (!physmem->object) {
		RuntimeError("Physical memory object must be initialized");
		return -1;
	}
	
	self->object = new PPCMMU(physmem->object);
	if (!self->object) {
		PyErr_NoMemory();
		return -1;
	}
	self->super.object = self->object;
	
	Py_INCREF(physmem);
	self->physmem = physmem;
	return 0;
}

PyObject *PPCMMU_setRpnSize(PPCMMUObj *self, PyObject *arg) {
	CHECK_INIT(self->object);

	int bits = PyLong_AsLong(arg);
	if (PyErr_Occurred()) return NULL;
	
	self->object->setRpnSize(bits);
	Py_RETURN_NONE;
}

PyObject *PPCMMU_setDbatu(PPCMMUObj *self, PyObject *args) {
	CHECK_INIT(self->object);
	
	int index;
	uint32_t value;
	if (!PyArg_ParseTuple(args, "iI", &index, &value)) return NULL;

	if (index < 0 || index > 7) {
		ValueError("Invalid dbatu index");
		return NULL;
	}
	
	self->object->dbatu[index] = value;
	self->object->invalidateCache();
	Py_RETURN_NONE;
}

PyObject *PPCMMU_setDbatl(PPCMMUObj *self, PyObject *args) {
	CHECK_INIT(self->object);
	
	int index;
	uint32_t value;
	if (!PyArg_ParseTuple(args, "iI", &index, &value)) return NULL;
	
	if (index < 0 || index > 7) {
		ValueError("Invalid dbatl index");
		return NULL;
	}
	
	self->object->dbatl[index] = value;
	self->object->invalidateCache();
	Py_RETURN_NONE;
}

PyObject *PPCMMU_setIbatu(PPCMMUObj *self, PyObject *args) {
	CHECK_INIT(self->object);
	
	int index;
	uint32_t value;
	if (!PyArg_ParseTuple(args, "iI", &index, &value)) return NULL;
	
	if (index < 0 || index > 7) {
		ValueError("Invalid ibatu index");
		return NULL;
	}
	
	self->object->ibatu[index] = value;
	self->object->invalidateCache();
	Py_RETURN_NONE;
}

PyObject *PPCMMU_setIbatl(PPCMMUObj *self, PyObject *args) {
	CHECK_INIT(self->object);
	
	int index;
	uint32_t value;
	if (!PyArg_ParseTuple(args, "iI", &index, &value)) return NULL;
	
	if (index < 0 || index > 7) {
		ValueError("Invalid ibatl index");
		return NULL;
	}
	
	self->object->ibatl[index] = value;
	self->object->invalidateCache();
	Py_RETURN_NONE;
}

PyObject *PPCMMU_getDbatu(PPCMMUObj *self, PyObject *arg) {
	CHECK_INIT(self->object);
	
	int index = PyLong_AsLong(arg);
	if (PyErr_Occurred()) return NULL;
	
	if (index < 0 || index > 7) {
		ValueError("Invalid dbatu index");
		return NULL;
	}
	
	return PyLong_FromUnsignedLong(self->object->dbatu[index]);
}

PyObject *PPCMMU_getDbatl(PPCMMUObj *self, PyObject *arg) {
	CHECK_INIT(self->object);
	
	int index = PyLong_AsLong(arg);
	if (PyErr_Occurred()) return NULL;
	
	if (index < 0 || index > 7) {
		ValueError("Invalid dbatl index");
		return NULL;
	}
	
	return PyLong_FromUnsignedLong(self->object->dbatl[index]);
}

PyObject *PPCMMU_getIbatu(PPCMMUObj *self, PyObject *arg) {
	CHECK_INIT(self->object);
	
	int index = PyLong_AsLong(arg);
	if (PyErr_Occurred()) return NULL;
	
	if (index < 0 || index > 7) {
		ValueError("Invalid ibatu index");
		return NULL;
	}
	
	return PyLong_FromUnsignedLong(self->object->ibatu[index]);
}

PyObject *PPCMMU_getIbatl(PPCMMUObj *self, PyObject *arg) {
	CHECK_INIT(self->object);
	
	int index = PyLong_AsLong(arg);
	if (PyErr_Occurred()) return NULL;
	
	if (index < 0 || index > 7) {
		ValueError("Invalid ibatl index");
		return NULL;
	}
	
	return PyLong_FromUnsignedLong(self->object->ibatl[index]);
}

PyObject *PPCMMU_setSr(PPCMMUObj *self, PyObject *args) {
	CHECK_INIT(self->object);
	
	int index;
	uint32_t value;
	if (!PyArg_ParseTuple(args, "iI", &index, &value)) return NULL;
	
	if (index < 0 || index > 15) {
		ValueError("Invalid sr index");
		return NULL;
	}
	
	self->object->sr[index] = value;
	self->object->invalidateCache();
	Py_RETURN_NONE;
}

PyObject *PPCMMU_getSr(PPCMMUObj *self, PyObject *arg) {
	CHECK_INIT(self->object);
	
	int index = PyLong_AsLong(arg);
	if (PyErr_Occurred()) return NULL;
	
	if (index < 0 || index > 15) {
		ValueError("Invalid sr index");
		return NULL;
	}
	
	return PyLong_FromUnsignedLong(self->object->sr[index]);
}

PyObject *PPCMMU_getSdr1(PPCMMUObj *self, PyObject *args) {
	CHECK_INIT(self->object);
	return PyLong_FromUnsignedLong(self->object->sdr1);
}

PyObject *PPCMMU_setSdr1(PPCMMUObj *self, PyObject *arg) {
	CHECK_INIT(self->object);
	
	uint32_t value = PyLong_AsUnsignedLong(arg);
	if (PyErr_Occurred()) return NULL;
	
	self->object->sdr1 = value;
	self->object->invalidateCache();
	Py_RETURN_NONE;
}

PyMethodDef PPCMMU_methods[] = {
	{"set_rpn_size", (PyCFunction)PPCMMU_setRpnSize, METH_O, NULL},
	{"set_dbatu", (PyCFunction)PPCMMU_setDbatu, METH_VARARGS, NULL},
	{"set_dbatl", (PyCFunction)PPCMMU_setDbatl, METH_VARARGS, NULL},
	{"set_ibatu", (PyCFunction)PPCMMU_setIbatu, METH_VARARGS, NULL},
	{"set_ibatl", (PyCFunction)PPCMMU_setIbatl, METH_VARARGS, NULL},
	{"get_dbatu", (PyCFunction)PPCMMU_getDbatu, METH_O, NULL},
	{"get_dbatl", (PyCFunction)PPCMMU_getDbatl, METH_O, NULL},
	{"get_ibatu", (PyCFunction)PPCMMU_getIbatu, METH_O, NULL},
	{"get_ibatl", (PyCFunction)PPCMMU_getIbatl, METH_O, NULL},
	{"set_sr", (PyCFunction)PPCMMU_setSr, METH_VARARGS, NULL},
	{"get_sr", (PyCFunction)PPCMMU_getSr, METH_O, NULL},
	{"get_sdr1", (PyCFunction)PPCMMU_getSdr1, METH_NOARGS, NULL},
	{"set_sdr1", (PyCFunction)PPCMMU_setSdr1, METH_O, NULL},
	{NULL}
};

PyTypeObject PPCMMUType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pyemu.PPCMMU",
	sizeof(PPCMMUObj),
	0, //itemsize
	(destructor)PPCMMU_dealloc, //dealloc
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
	(traverseproc)PPCMMU_traverse, //traverse
	(inquiry)PPCMMU_clear, //clear
	0, //richcompare
	0, //weaklistoffset
	0, //iter
	0, //iternext
	PPCMMU_methods, //methods
	0, //members
	0, //getset
	&IVirtMemType, //base
	0, //dict
	0, //descr_get
	0, //descr_set
	0, //dictoffset
	(initproc)PPCMMU_init, //init
	0, //allocate
	PPCMMU_new, //new
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

bool PPCMMUType_Init() {
	return PyType_Ready(&PPCMMUType) == 0;
}
