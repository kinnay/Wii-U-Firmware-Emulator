
#include <Python.h>
#include "type_ppclock.h"
#include "type_ppccore.h"
#include "class_ppccore.h"
#include "errors.h"
#include "common.h"

int PPCCore_clear(PPCCoreObj *self) {
	Py_CLEAR(self->lockMgr);
	Py_CLEAR(self->sprReadCB);
	Py_CLEAR(self->sprWriteCB);
	Py_CLEAR(self->msrWriteCB);
	Py_CLEAR(self->srReadCB);
	Py_CLEAR(self->srWriteCB);
	return 0;
}

int PPCCore_traverse(PPCCoreObj *self, visitproc visit, void *arg) {
	Py_VISIT(self->lockMgr);
	Py_VISIT(self->sprReadCB);
	Py_VISIT(self->sprWriteCB);
	Py_VISIT(self->msrWriteCB);
	Py_VISIT(self->srReadCB);
	Py_VISIT(self->srWriteCB);
	return 0;
}

void PPCCore_dealloc(PPCCoreObj *self) {
	if (self->object) {
		delete self->object;
	}
	PyObject_GC_UnTrack(self);
	PPCCore_clear(self);
	Py_TYPE(self)->tp_free((PyObject *)self);
}

PyObject *PPCCore_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
	PPCCoreObj *self = (PPCCoreObj *)type->tp_alloc(type, 0);
	if (self) {
		self->object = NULL;
		self->lockMgr = NULL;
		self->sprReadCB = NULL;
		self->sprWriteCB = NULL;
		self->msrWriteCB = NULL;
		self->srReadCB = NULL;
		self->srWriteCB = NULL;
	}
	return (PyObject *)self;
}

int PPCCore_init(PPCCoreObj *self, PyObject *args, PyObject *kwargs) {
	CHECK_NOT_INIT(self->object);
	
	PPCLockObj *lockMgr;
	if (!PyArg_ParseTuple(args, "O!", &PPCLockType, &lockMgr)) {
		return -1;
	}
	
	if (!lockMgr->object) {
		RuntimeError("Reservation object must be initialized");
		return -1;
	}
	
	self->object = new PPCCore(lockMgr->object);
	if (!self->object) {
		PyErr_NoMemory();
		return -1;
	}
	
	Py_INCREF(lockMgr);
	self->lockMgr = lockMgr;
	return 0;
}

PyObject *PPCCore_getReg(PPCCoreObj *self, PyObject *args) {
	CHECK_INIT(self->object);
	
	int reg;
	if (!PyArg_ParseTuple(args, "i", &reg)) return NULL;
	if (reg < 0 || reg > 31) {
		ValueError("Invalid register");
		return NULL;
	}
	return PyLong_FromUnsignedLong(self->object->regs[reg]);
}

PyObject *PPCCore_setReg(PPCCoreObj *self, PyObject *args) {
	CHECK_INIT(self->object);
	
	int reg;
	uint32_t value;
	if (!PyArg_ParseTuple(args, "iI", &reg, &value)) return NULL;
	if (reg < 0 || reg > 31) {
		ValueError("Invalid register");
		return NULL;
	}
	self->object->regs[reg] = value;
	Py_RETURN_NONE;
}

PyObject *PPCCore_getSpr(PPCCoreObj *self, PyObject *args) {
	CHECK_INIT(self->object);
	
	int reg;
	if (!PyArg_ParseTuple(args, "i", &reg)) return NULL;
	if (reg < 0 || reg > 0x3FF) {
		ValueError("Invalid register");
		return NULL;
	}
	
	uint32_t value;
	if (!self->object->getSpr((PPCCore::SPR)reg, &value)) {
		return NULL;
	}
	return PyLong_FromUnsignedLong(value);
}

PyObject *PPCCore_setSpr(PPCCoreObj *self, PyObject *args) {
	CHECK_INIT(self->object);
	
	int reg;
	uint32_t value;
	if (!PyArg_ParseTuple(args, "iI", &reg, &value)) return NULL;
	if (reg < 0 || reg > 0x3FF) {
		ValueError("Invalid register");
		return NULL;
	}
	
	if (!self->object->setSpr((PPCCore::SPR)reg, value)) {
		return NULL;
	}
	Py_RETURN_NONE;
}

PyObject *PPCCore_getPc(PPCCoreObj *self, PyObject *args) {
	CHECK_INIT(self->object);
	return PyLong_FromUnsignedLong(self->object->pc);
}

PyObject *PPCCore_setPc(PPCCoreObj *self, PyObject *arg) {
	CHECK_INIT(self->object);
	
	uint32_t value = PyLong_AsUnsignedLong(arg);
	if (PyErr_Occurred()) return NULL;
	
	self->object->pc = value;
	Py_RETURN_NONE;
}

PyObject *PPCCore_getCr(PPCCoreObj *self, PyObject *args) {
	CHECK_INIT(self->object);
	return PyLong_FromUnsignedLong(self->object->cr);
}

PyObject *PPCCore_setCr(PPCCoreObj *self, PyObject *arg) {
	CHECK_INIT(self->object);
	
	uint32_t value = PyLong_AsUnsignedLong(arg);
	if (PyErr_Occurred()) return NULL;
	
	self->object->cr = value;
	Py_RETURN_NONE;
}

PyObject *PPCCore_getMsr(PPCCoreObj *self, PyObject *args) {
	CHECK_INIT(self->object);
	return PyLong_FromUnsignedLong(self->object->msr);
}

PyObject *PPCCore_setMsr(PPCCoreObj *self, PyObject *arg) {
	CHECK_INIT(self->object);
	
	uint32_t value = PyLong_AsUnsignedLong(arg);
	if (PyErr_Occurred()) return NULL;
	
	if (!self->object->setMsr(value)) {
		return NULL;
	}
	Py_RETURN_NONE;
}

PyObject *PPCCore_getTb(PPCCoreObj *self, PyObject *args) {
	CHECK_INIT(self->object);
	return PyLong_FromUnsignedLongLong(self->object->tb);
}

PyObject *PPCCore_setTb(PPCCoreObj *self, PyObject *arg) {
	CHECK_INIT(self->object);
	
	uint64_t value = PyLong_AsUnsignedLongLong(arg);
	if (PyErr_Occurred()) return NULL;
	
	self->object->tb = value;
	Py_RETURN_NONE;
}

PyObject *PPCCore_triggerException(PPCCoreObj *self, PyObject *arg) {
	CHECK_INIT(self->object);
	
	int type = PyLong_AsLong(arg);
	if (PyErr_Occurred()) return NULL;
	
	if (type != PPCCore::DSI && type != PPCCore::ISI &&
		type != PPCCore::ExternalInterrupt && type != PPCCore::Decrementer &&
		type != PPCCore::SystemCall && type != PPCCore::ICI) {
		ValueError("Invalid exception type");
		return NULL;
	}
	
	if (!self->object->triggerException((PPCCore::ExceptionType)type)) {
		return NULL;
	}
	Py_RETURN_NONE;
}

PyObject *PPCCore_onSprRead(PPCCoreObj *self, PyObject *arg) {
	CHECK_INIT(self->object);
	if (!PyCallable_Check(arg)) {
		TypeError("Parameter must be callable");
		return NULL;
	}
	
	Py_INCREF(arg);
	Py_XDECREF(self->sprReadCB);
	self->sprReadCB = arg;
	
	self->object->setSprReadCB(
		[arg] (PPCCore::SPR spr, uint32_t *value) {
			PyObject *args = Py_BuildValue("(i)", spr);
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

PyObject *PPCCore_onSprWrite(PPCCoreObj *self, PyObject *arg) {
	CHECK_INIT(self->object);
	if (!PyCallable_Check(arg)) {
		TypeError("Parameter must be callable");
		return NULL;
	}
	
	Py_INCREF(arg);
	Py_XDECREF(self->sprWriteCB);
	self->sprWriteCB = arg;
	
	self->object->setSprWriteCB(
		[arg] (PPCCore::SPR spr, uint32_t value) {
			PyObject *args = Py_BuildValue("(iI)", spr, value);
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

PyObject *PPCCore_onMsrWrite(PPCCoreObj *self, PyObject *arg) {
	CHECK_INIT(self->object);
	if (!PyCallable_Check(arg)) {
		TypeError("Parameter must be callable");
		return NULL;
	}
	
	Py_INCREF(arg);
	Py_XDECREF(self->msrWriteCB);
	self->msrWriteCB = arg;
	
	self->object->setMsrWriteCB(
		[arg] (uint32_t value) {
			PyObject *args = Py_BuildValue("(I)", value);
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

PyObject *PPCCore_onSrWrite(PPCCoreObj *self, PyObject *arg) {
	CHECK_INIT(self->object);
	if (!PyCallable_Check(arg)) {
		TypeError("Parameter must be callable");
		return NULL;
	}
	
	Py_INCREF(arg);
	Py_XDECREF(self->srWriteCB);
	self->srWriteCB = arg;
	
	self->object->setSrWriteCB(
		[arg] (int index, uint32_t value) {
			PyObject *args = Py_BuildValue("(iI)", index, value);
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

PyObject *PPCCore_onSrRead(PPCCoreObj *self, PyObject *arg) {
	CHECK_INIT(self->object);
	if (!PyCallable_Check(arg)) {
		TypeError("Parameter must be callable");
		return NULL;
	}
	
	Py_INCREF(arg);
	Py_XDECREF(self->srReadCB);
	self->srReadCB = arg;
	
	self->object->setSrReadCB(
		[arg] (int index, uint32_t *value) {
			PyObject *args = Py_BuildValue("(i)", index);
			if (!args) return false;
			
			PyObject *result = PyObject_CallObject(arg, args);
			Py_DECREF(args);
			
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

PyMethodDef PPCCore_methods[] = {
	{"reg", (PyCFunction)PPCCore_getReg, METH_VARARGS, NULL},
	{"setreg", (PyCFunction)PPCCore_setReg, METH_VARARGS, NULL},
	{"spr", (PyCFunction)PPCCore_getSpr, METH_VARARGS, NULL},
	{"setspr", (PyCFunction)PPCCore_setSpr, METH_VARARGS, NULL},
	{"pc", (PyCFunction)PPCCore_getPc, METH_NOARGS, NULL},
	{"setpc", (PyCFunction)PPCCore_setPc, METH_O, NULL},
	{"cr", (PyCFunction)PPCCore_getCr, METH_NOARGS, NULL},
	{"setcr", (PyCFunction)PPCCore_setCr, METH_O, NULL},
	{"msr", (PyCFunction)PPCCore_getMsr, METH_NOARGS, NULL},
	{"setmsr", (PyCFunction)PPCCore_setMsr, METH_O, NULL},
	{"tb", (PyCFunction)PPCCore_getTb, METH_NOARGS, NULL},
	{"settb", (PyCFunction)PPCCore_setTb, METH_O, NULL},
	{"trigger_exception", (PyCFunction)PPCCore_triggerException, METH_O, NULL},
	{"on_spr_read", (PyCFunction)PPCCore_onSprRead, METH_O, NULL},
	{"on_spr_write", (PyCFunction)PPCCore_onSprWrite, METH_O, NULL},
	{"on_msr_write", (PyCFunction)PPCCore_onMsrWrite, METH_O, NULL},
	{"on_sr_read", (PyCFunction)PPCCore_onSrRead, METH_O, NULL},
	{"on_sr_write", (PyCFunction)PPCCore_onSrWrite, METH_O, NULL},
	{NULL}
};

PyTypeObject PPCCoreType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pyemu.PPCCore",
	sizeof(PPCCoreObj),
	0, //itemsize
	(destructor)PPCCore_dealloc, //dealloc
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
	"Represents a PowerPC core", //doc
	(traverseproc)PPCCore_traverse, //traverse
	(inquiry)PPCCore_clear, //clear
	0, //richcompare
	0, //weaklistoffset
	0, //iter
	0, //iternext
	PPCCore_methods, //methods
	0, //members
	0, //getset
	0, //base
	0, //dict
	0, //descr_get
	0, //descr_set
	0, //dictoffset
	(initproc)PPCCore_init, //init
	0, //allocate
	PPCCore_new, //new
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

bool PPCCoreType_Init() {
	PyObject *dict = PyDict_New();
	
	PyDict_SetItemString(dict, "r0", PyLong_FromLong(0));
	PyDict_SetItemString(dict, "r1", PyLong_FromLong(1));
	PyDict_SetItemString(dict, "r2", PyLong_FromLong(2));
	PyDict_SetItemString(dict, "r3", PyLong_FromLong(3));
	PyDict_SetItemString(dict, "r4", PyLong_FromLong(4));
	PyDict_SetItemString(dict, "r5", PyLong_FromLong(5));
	PyDict_SetItemString(dict, "r6", PyLong_FromLong(6));
	PyDict_SetItemString(dict, "r7", PyLong_FromLong(7));
	PyDict_SetItemString(dict, "r8", PyLong_FromLong(8));
	PyDict_SetItemString(dict, "r9", PyLong_FromLong(9));
	PyDict_SetItemString(dict, "r10", PyLong_FromLong(10));
	PyDict_SetItemString(dict, "r11", PyLong_FromLong(11));
	PyDict_SetItemString(dict, "r12", PyLong_FromLong(12));
	PyDict_SetItemString(dict, "r13", PyLong_FromLong(13));
	PyDict_SetItemString(dict, "r14", PyLong_FromLong(14));
	PyDict_SetItemString(dict, "r15", PyLong_FromLong(15));
	PyDict_SetItemString(dict, "r16", PyLong_FromLong(16));
	PyDict_SetItemString(dict, "r17", PyLong_FromLong(17));
	PyDict_SetItemString(dict, "r18", PyLong_FromLong(18));
	PyDict_SetItemString(dict, "r19", PyLong_FromLong(19));
	PyDict_SetItemString(dict, "r20", PyLong_FromLong(20));
	PyDict_SetItemString(dict, "r21", PyLong_FromLong(21));
	PyDict_SetItemString(dict, "r22", PyLong_FromLong(22));
	PyDict_SetItemString(dict, "r23", PyLong_FromLong(23));
	PyDict_SetItemString(dict, "r24", PyLong_FromLong(24));
	PyDict_SetItemString(dict, "r25", PyLong_FromLong(25));
	PyDict_SetItemString(dict, "r26", PyLong_FromLong(26));
	PyDict_SetItemString(dict, "r27", PyLong_FromLong(27));
	PyDict_SetItemString(dict, "r28", PyLong_FromLong(28));
	PyDict_SetItemString(dict, "r29", PyLong_FromLong(29));
	PyDict_SetItemString(dict, "r30", PyLong_FromLong(30));
	PyDict_SetItemString(dict, "r31", PyLong_FromLong(31));
	
	PyDict_SetItemString(dict, "XER", PyLong_FromLong(PPCCore::XER));
	PyDict_SetItemString(dict, "LR", PyLong_FromLong(PPCCore::LR));
	PyDict_SetItemString(dict, "CTR", PyLong_FromLong(PPCCore::CTR));
	PyDict_SetItemString(dict, "DSISR", PyLong_FromLong(PPCCore::DSISR));
	PyDict_SetItemString(dict, "DAR", PyLong_FromLong(PPCCore::DAR));
	PyDict_SetItemString(dict, "SRR0", PyLong_FromLong(PPCCore::SRR0));
	PyDict_SetItemString(dict, "SRR1", PyLong_FromLong(PPCCore::SRR1));
	PyDict_SetItemString(dict, "UPIR", PyLong_FromLong(PPCCore::UPIR));
	
	PyDict_SetItemString(dict, "DSI", PyLong_FromLong(PPCCore::DSI));
	PyDict_SetItemString(dict, "ISI", PyLong_FromLong(PPCCore::ISI));
	PyDict_SetItemString(dict, "EXTERNAL_INTERRUPT", PyLong_FromLong(PPCCore::ExternalInterrupt));
	PyDict_SetItemString(dict, "DECREMENTER", PyLong_FromLong(PPCCore::Decrementer));
	PyDict_SetItemString(dict, "SYSTEM_CALL", PyLong_FromLong(PPCCore::SystemCall));
	PyDict_SetItemString(dict, "ICI", PyLong_FromLong(PPCCore::ICI));
	
	PPCCoreType.tp_dict = dict;
	return PyType_Ready(&PPCCoreType) == 0;
}
