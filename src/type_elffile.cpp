
#include <Python.h>
#include "type_elffile.h"
#include "type_elfprogram.h"
#include "class_elffile.h"
#include "errors.h"
#include "common.h"

void ELFFile_dealloc(ELFFileObj *self) {
	if (self->object) {
		delete self->object;
	}
	Py_TYPE(self)->tp_free((PyObject *)self);
}

PyObject *ELFFile_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
	ELFFileObj *self = (ELFFileObj *)type->tp_alloc(type, 0);
	if (self) {
		self->object = NULL;
	}
	return (PyObject *)self;
}

int ELFFile_init(ELFFileObj *self, PyObject *args, PyObject *kwargs) {
	CHECK_NOT_INIT(self->object)
	
	self->object = new ELFFile();
	if (!self->object) {
		PyErr_NoMemory();
		return -1;
	}
	
	const void *buffer;
	uint32_t length;
	if (!PyArg_ParseTuple(args, "y#", &buffer, &length)) {
		delete self->object;
		self->object = NULL;
		return -1;
	}

	if (!self->object->init(buffer, length)) {
		delete self->object;
		self->object = NULL;
		return -1;
	}
	return 0;
}

PyObject *ELFFile_getBitSize(ELFFileObj *self, void *closure) {
	CHECK_INIT(self->object)
	return PyLong_FromLong(self->object->header->bitsize);
}

PyObject *ELFFile_getEndianness(ELFFileObj *self, void *closure) {
	CHECK_INIT(self->object)
	return PyLong_FromLong(self->object->header->endianness);
}

PyObject *ELFFile_getOSAbi(ELFFileObj *self, void *closure) {
	CHECK_INIT(self->object)
	return PyLong_FromLong(self->object->header->osType);
}

PyObject *ELFFile_getOSVersion(ELFFileObj *self, void *closure) {
	CHECK_INIT(self->object)
	return PyLong_FromLong(self->object->header->osVersion);
}

PyObject *ELFFile_getType(ELFFileObj *self, void *closure) {
	CHECK_INIT(self->object)
	return PyLong_FromLong(self->object->header->type);
}

PyObject *ELFFile_getMachine(ELFFileObj *self, void *closure) {
	CHECK_INIT(self->object)
	return PyLong_FromLong(self->object->header->machine);
}

PyObject *ELFFile_getEntryPoint(ELFFileObj *self, void *closure) {
	CHECK_INIT(self->object)
	return PyLong_FromUnsignedLong(self->object->header->entry);
}

PyObject *ELFFile_getFlags(ELFFileObj *self, void *closure) {
	CHECK_INIT(self->object)
	return PyLong_FromUnsignedLong(self->object->header->flags);
}

PyObject *ELFFile_getPrograms(ELFFileObj *self, void *closure) {
	CHECK_INIT(self->object)
	PyObject *list = PyList_New(self->object->programs.size());
	for (uint32_t i = 0; i < self->object->programs.size(); i++) {
		ELFProgramObj *program = PyObject_GC_New(ELFProgramObj, &ELFProgramType);
		Py_INCREF(self);
		program->parent = (PyObject *)self;
		program->object = self->object->programs.at(i);
		PyList_SET_ITEM(list, i, (PyObject *)program);
	}
	return list;
}

PyObject *ELFFile_getSections(ELFFileObj *self, void *closure) {
	CHECK_INIT(self->object)
	NotImplementedError("TODO: ELFFile.sections");
	return NULL;
}

PyGetSetDef ELFFile_getSetters[] = {
	{"bitsize", (getter)ELFFile_getBitSize, NULL, "32-bit or 64-bit", NULL},
	{"endianness", (getter)ELFFile_getEndianness, NULL, "Little-endian or big-endian", NULL},
	{"os_abi", (getter)ELFFile_getOSAbi, NULL, NULL, NULL},
	{"os_version", (getter)ELFFile_getOSVersion, NULL, NULL, NULL},
	{"type", (getter)ELFFile_getType, NULL, "Relocatable / executable / shared / core", NULL},
	{"machine", (getter)ELFFile_getMachine, NULL, NULL, NULL},
	{"entry_point", (getter)ELFFile_getEntryPoint, NULL, NULL, NULL},
	{"flags", (getter)ELFFile_getFlags, NULL, NULL, NULL},
	{"programs", (getter)ELFFile_getPrograms, NULL, NULL, NULL},
	{"sections", (getter)ELFFile_getSections, NULL, NULL, NULL},
	{NULL}
};

PyTypeObject ELFFileType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pyemu.ELFFile",
	sizeof(ELFFileObj),
	0, //itemsize
	(destructor)ELFFile_dealloc, //dealloc
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
	"An ELF file object", //doc
	0, //traverse
	0, //clear
	0, //richcompare
	0, //weaklistoffset
	0, //iter
	0, //iternext
	0, //methods
	0, //members
	ELFFile_getSetters, //getset
	0, //base
	0, //dict
	0, //descr_get
	0, //descr_set
	0, //dictoffset
	(initproc)ELFFile_init, //init
	0, //allocate
	ELFFile_new, //new
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

bool ELFFileType_Init() {
	PyObject *dict = PyDict_New();
	
	PyDict_SetItemString(dict, "X32", PyLong_FromLong(ELFHeader::X32));
	PyDict_SetItemString(dict, "X64", PyLong_FromLong(ELFHeader::X64));
	
	PyDict_SetItemString(dict, "LITTLE_ENDIAN", PyLong_FromLong(ELFHeader::Little));
	PyDict_SetItemString(dict, "BIG_ENDIAN", PyLong_FromLong(ELFHeader::Big));
	
	PyDict_SetItemString(dict, "RELOCATABLE", PyLong_FromLong(ELFHeader::Relocatable));
	PyDict_SetItemString(dict, "EXECUTABLE", PyLong_FromLong(ELFHeader::Executable));
	PyDict_SetItemString(dict, "SHARED", PyLong_FromLong(ELFHeader::Shared));
	PyDict_SetItemString(dict, "CORE", PyLong_FromLong(ELFHeader::Core));
	
	ELFFileType.tp_dict = dict;
	return PyType_Ready(&ELFFileType) == 0;
}

