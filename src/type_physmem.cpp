
#include <Python.h>
#include "type_iphysmem.h"
#include "type_physmem.h"
#include "class_physmem.h"
#include "errors.h"
#include "common.h"

int PhysMem_clear(PhysMemObj *self) {
	for (PyObject *i : self->callbacks) {
		Py_CLEAR(i);
	}
	return 0;
}

int PhysMem_traverse(PhysMemObj *self, visitproc visit, void *arg) {
	for (PyObject *i : self->callbacks) {
		Py_VISIT(i);
	}
	return 0;
}

void PhysMem_dealloc(PhysMemObj *self) {
	if (self->object) {
		delete self->object;
	}
	PyObject_GC_UnTrack(self);
	PhysMem_clear(self);
	Py_TYPE(self)->tp_free((PyObject *)self);
}

PyObject *PhysMem_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
	PhysMemObj *self = (PhysMemObj *)IPhysMemType.tp_new(type, args, kwargs);
	if (self) {
		self->object = NULL;
	}
	return (PyObject *)self;
}

int PhysMem_init(PhysMemObj *self, PyObject *args, PyObject *kwargs) {
	CHECK_NOT_INIT(self->object);
	
	self->object = new PhysicalMemory();
	if (!self->object) {
		PyErr_NoMemory();
		return -1;
	}
	self->super.object = self->object;
	return 0;
}

PyObject *PhysMem_addRange(PhysMemObj *self, PyObject *args) {
	CHECK_INIT(self->object);

	uint32_t start, length;
	if (!PyArg_ParseTuple(args, "II", &start, &length)) return NULL;
	if (!self->object->addRange(start, length)) return NULL;
	Py_RETURN_NONE;
}

PyObject *PhysMem_addSpecial(PhysMemObj *self, PyObject *args) {
	CHECK_INIT(self->object);
	
	uint32_t start, length;
	PyObject *readCB, *writeCB;
	if (!PyArg_ParseTuple(args, "IIOO", &start, &length, &readCB, &writeCB)) {
		return NULL;
	}
	
	if (!PyCallable_Check(readCB) || !PyCallable_Check(writeCB)) {
		TypeError("Callbacks must be callable");
		return NULL;
	}
	
	if (!self->object->addSpecial(start, length,
		[readCB] (uint32_t addr, void *data, uint32_t length) -> bool {
			PyObject *args = Py_BuildValue("(II)", addr, length);
			if (!args) return false;
			
			PyObject *result = PyObject_CallObject(readCB, args);
			Py_DECREF(args);
			
			if (!result) return false;
			
			char *buffer;
			int buflen;
			if (PyBytes_AsStringAndSize(result, &buffer, &buflen) == -1) {
				Py_DECREF(result);
				return false;
			}
			
			Py_DECREF(result);
			if (buflen != length) {
				ValueError("Returned length doesn't match requested length");
				return false;
			}
			
			memcpy(data, buffer, buflen);
			return true;
		},
		[writeCB] (uint32_t addr, const void *data, uint32_t length) -> bool {
			PyObject *args = Py_BuildValue("(Iy#)", addr, data, length);
			if (!args) return false;
			
			PyObject *result = PyObject_CallObject(writeCB, args);
			Py_DECREF(args);
			
			if (!result) return false;
			Py_DECREF(result);
			return true;
		}
	)) {
		return NULL;
	}
	
	Py_INCREF(readCB);
	Py_INCREF(writeCB);
	self->callbacks.push_back(readCB);
	self->callbacks.push_back(writeCB);
	
	Py_RETURN_NONE;
}

PyMethodDef PhysMem_methods[] = {
	{"add_range", (PyCFunction)PhysMem_addRange, METH_VARARGS, NULL},
	{"add_special", (PyCFunction)PhysMem_addSpecial, METH_VARARGS, NULL},
	{NULL}
};

PyTypeObject PhysMemType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pyemu.PhysicalMemory", 
	sizeof(PhysMemObj),
	0, //itemsize
	(destructor)PhysMem_dealloc, //dealloc
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
	(traverseproc)PhysMem_traverse, //traverse
	(inquiry)PhysMem_clear, //clear
	0, //richcompare
	0, //weaklistoffset
	0, //iter
	0, //iternext
	PhysMem_methods, //methods
	0, //members
	0, //getset
	&IPhysMemType, //base
	0, //dict
	0, //descr_get
	0, //descr_set
	0, //dictoffset
	(initproc)PhysMem_init, //init
	0, //allocate
	PhysMem_new, //new
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

bool PhysMemType_Init() {
	return PyType_Ready(&PhysMemType) == 0;
}
