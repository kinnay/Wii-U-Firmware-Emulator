
#include <Python.h>
#include <algorithm>
#include "type_interpreter.h"
#include "type_arminterp.h"
#include "type_armcore.h"
#include "type_physmem.h"
#include "type_ivirtmem.h"
#include "class_arminterp.h"
#include "errors.h"
#include "common.h"

int ARMInterp_clear(ARMInterpObj *self) {
	InterpreterType.tp_clear((PyObject *)self);
	Py_CLEAR(self->core);
	Py_CLEAR(self->coprocReadCB);
	Py_CLEAR(self->coprocWriteCB);
	Py_CLEAR(self->softwareInterruptCB);
	Py_CLEAR(self->undefinedCB);
	return 0;
}

int ARMInterp_traverse(ARMInterpObj *self, visitproc visit, void *arg) {
	InterpreterType.tp_traverse((PyObject *)self, visit, arg);
	Py_VISIT(self->core);
	Py_VISIT(self->coprocReadCB);
	Py_VISIT(self->coprocWriteCB);
	Py_VISIT(self->softwareInterruptCB);
	Py_VISIT(self->undefinedCB);
	return 0;
}

void ARMInterp_dealloc(ARMInterpObj *self) {
	if (self->object) {
		delete self->object;
	}
	PyObject_GC_UnTrack(self);
	ARMInterp_clear(self);
	Py_TYPE(self)->tp_free((PyObject *)self);
}

PyObject *ARMInterp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
	ARMInterpObj *self = (ARMInterpObj *)InterpreterType.tp_new(type, args, kwargs);
	if (self) {
		self->object = NULL;
		self->coprocReadCB = NULL;
		self->coprocWriteCB = NULL;
		self->softwareInterruptCB = NULL;
		self->undefinedCB = NULL;
	}
	return (PyObject *)self;
}

int ARMInterp_init(ARMInterpObj *self, PyObject *args, PyObject *kwargs) {
	CHECK_NOT_INIT(self->object);
	
	ARMCoreObj *core;
	PhysMemObj *physmem;
	IVirtMemObj *virtmem;
	if (!PyArg_ParseTuple(args, "O!O!O!", &ARMCoreType, &core,
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
		return -1;
	}
	
	self->object = new ARMInterpreter(core->object, physmem->object, virtmem->object);
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

PyObject *ARMInterp_onCoprocRead(ARMInterpObj *self, PyObject *arg) {
	CHECK_INIT(self->object);
	if (!PyCallable_Check(arg)) {
		TypeError("Parameter must be callable");
		return NULL;
	}
	
	Py_INCREF(arg);
	Py_XDECREF(self->coprocReadCB);
	self->coprocReadCB = arg;
	
	self->object->setCoprocReadCB(
		[arg] (int coproc, int opc, uint32_t *value, int rn, int rm, int type) -> bool {
			PyObject *args = Py_BuildValue("(iiiii)", coproc, opc, rn, rm, type);
			if (!args) return false;

			PyObject *result = PyObject_CallObject(arg, args);
			Py_DECREF(args);

			if (!result) {
				return false;
			}
			
			uint32_t outval = PyLong_AsUnsignedLong(result);
			Py_DECREF(result);
			
			if (PyErr_Occurred()) {
				return false;
			}
			*value = outval;
			return true;
		}
	);

	Py_RETURN_NONE;
}

PyObject *ARMInterp_onCoprocWrite(ARMInterpObj *self, PyObject *arg) {
	CHECK_INIT(self->object);
	if (!PyCallable_Check(arg)) {
		TypeError("Parameter must be callable");
		return NULL;
	}
	
	Py_INCREF(arg);
	Py_XDECREF(self->coprocWriteCB);
	self->coprocWriteCB = arg;
	
	self->object->setCoprocWriteCB(
		[arg] (int coproc, int opc, uint32_t value, int rn, int rm, int type) -> bool {
			PyObject *args = Py_BuildValue("(iiIiii)", coproc, opc, value, rn, rm, type);
			if (!args) return false;
			
			PyObject *result = PyObject_CallObject(arg, args);
			Py_DECREF(args);

			if (!result) {
				return false;
			}
			Py_DECREF(result);
			return true;
		}
	);

	Py_RETURN_NONE;
}

PyObject *ARMInterp_onSoftwareInterrupt(ARMInterpObj *self, PyObject *arg) {
	CHECK_INIT(self->object);
	if (!PyCallable_Check(arg)) {
		TypeError("Parameter must be callable");
		return NULL;
	}
	
	Py_INCREF(arg);
	Py_XDECREF(self->softwareInterruptCB);
	self->softwareInterruptCB = arg;
	
	self->object->setSoftwareInterruptCB(
		[arg] (int value) -> bool {
			PyObject *args = Py_BuildValue("(i)", value);
			if (!args) return false;
			
			PyObject *result = PyObject_CallObject(arg, args);
			Py_DECREF(args);
			
			if (!result) {
				return false;
			}
			Py_DECREF(result);
			return true;
		}
	);
	
	Py_RETURN_NONE;
}

PyObject *ARMInterp_onUndefinedInstruction(ARMInterpObj *self, PyObject *arg) {
	CHECK_INIT(self->object);
	if (!PyCallable_Check(arg)) {
		TypeError("Parameter must be callable");
		return NULL;
	}
	
	Py_INCREF(arg);
	Py_XDECREF(self->undefinedCB);
	self->undefinedCB = arg;
	
	self->object->setUndefinedCB(
		[arg] () -> bool {
			PyObject *result = PyObject_CallObject(arg, NULL);
			if (!result) return false;
			Py_DECREF(result);
			return true;
		}
	);
	
	Py_RETURN_NONE;
}

PyMethodDef ARMInterp_methods[] = {
	{"on_coproc_read", (PyCFunction)ARMInterp_onCoprocRead, METH_O, NULL},
	{"on_coproc_write", (PyCFunction)ARMInterp_onCoprocWrite, METH_O, NULL},
	{"on_software_interrupt", (PyCFunction)ARMInterp_onSoftwareInterrupt, METH_O, NULL},
	{"on_undefined_instruction", (PyCFunction)ARMInterp_onUndefinedInstruction, METH_O, NULL},
	{NULL}
};

PyTypeObject ARMInterpType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pyemu.ARMInterpreter", 
	sizeof(ARMInterpObj),
	0, //itemsize
	(destructor)ARMInterp_dealloc, //dealloc
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
	"An ARM interpreter", //doc
	(traverseproc)ARMInterp_traverse, //traverse
	(inquiry)ARMInterp_clear, //clear
	0, //richcompare
	0, //weaklistoffset
	0, //iter
	0, //iternext
	ARMInterp_methods, //methods
	0, //members
	0, //getset
	&InterpreterType, //base
	0, //dict
	0, //descr_get
	0, //descr_set
	0, //dictoffset
	(initproc)ARMInterp_init, //init
	0, //allocate
	ARMInterp_new, //new
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

bool ARMInterpType_Init() {
	return PyType_Ready(&ARMInterpType) == 0;
}
