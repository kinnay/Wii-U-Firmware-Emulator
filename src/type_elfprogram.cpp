
#include <Python.h>
#include "type_elfprogram.h"
#include "class_elffile.h"

int ELFProgram_clear(ELFProgramObj *self) {
	Py_CLEAR(self->parent);
	return 0;
}

int ELFProgram_traverse(ELFProgramObj *self, visitproc visit, void *arg) {
	Py_VISIT(self->parent);
	return 0;
}

void ELFProgram_dealloc(ELFProgramObj *self) {
	PyObject_GC_UnTrack(self);
	ELFProgram_clear(self);
	Py_TYPE(self)->tp_free((PyObject *)self);
}

PyObject *ELFProgram_getType(ELFProgramObj *self, void *closure) {
	return PyLong_FromUnsignedLong(self->object->type);
}

PyObject *ELFProgram_getFileOffs(ELFProgramObj *self, void *closure) {
	return PyLong_FromUnsignedLong(self->object->fileoffs);
}

PyObject *ELFProgram_getVirtAddr(ELFProgramObj *self, void *closure) {
	return PyLong_FromUnsignedLong(self->object->vaddr);
}

PyObject *ELFProgram_getPhysAddr(ELFProgramObj *self, void *closure) {
	return PyLong_FromUnsignedLong(self->object->paddr);
}

PyObject *ELFProgram_getFileSize(ELFProgramObj *self, void *closure) {
	return PyLong_FromUnsignedLong(self->object->filesize);
}

PyObject *ELFProgram_getMemSize(ELFProgramObj *self, void *closure) {
	return PyLong_FromUnsignedLong(self->object->memsize);
}

PyObject *ELFProgram_getFlags(ELFProgramObj *self, void *closure) {
	return PyLong_FromUnsignedLong(self->object->flags);
}

PyObject *ELFProgram_getAlignment(ELFProgramObj *self, void *closure) {
	return PyLong_FromUnsignedLong(self->object->alignment);
}

PyGetSetDef ELFProgram_getSetters[] = {
	{"type", (getter)ELFProgram_getType, NULL, NULL, NULL},
	{"fileoffs", (getter)ELFProgram_getFileOffs, NULL, NULL, NULL},
	{"virtaddr", (getter)ELFProgram_getVirtAddr, NULL, NULL, NULL},
	{"physaddr", (getter)ELFProgram_getPhysAddr, NULL, NULL, NULL},
	{"filesize", (getter)ELFProgram_getFileSize, NULL, NULL, NULL},
	{"memsize", (getter)ELFProgram_getMemSize, NULL, NULL, NULL},
	{"flags", (getter)ELFProgram_getFlags, NULL, NULL, NULL},
	{"alignment", (getter)ELFProgram_getAlignment, NULL, NULL, NULL},
	{NULL}
};

PyTypeObject ELFProgramType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pyemu.ELFProgram",
	sizeof(ELFProgramObj),
	0, //itemsize
	(destructor)ELFProgram_dealloc, //dealloc
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
	"An ELF file program", //doc
	(traverseproc)ELFProgram_traverse, //traverse
	(inquiry)ELFProgram_clear, //clear
	0, //richcompare
	0, //weaklistoffset
	0, //iter
	0, //iternext
	0, //methods
	0, //members
	ELFProgram_getSetters, //getset
	0, //base
	0, //dict
	0, //descr_get
	0, //descr_set
	0, //dictoffset
	0, //init
	0, //allocate
	0, //new
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

bool ELFProgramType_Init() {
	PyObject *dict = PyDict_New();
	
	PyDict_SetItemString(dict, "PT_NULL", PyLong_FromLong(ELFProgram::PT_NULL));
	PyDict_SetItemString(dict, "PT_LOAD", PyLong_FromLong(ELFProgram::PT_LOAD));
	PyDict_SetItemString(dict, "PT_DYNAMIC", PyLong_FromLong(ELFProgram::PT_DYNAMIC));
	PyDict_SetItemString(dict, "PT_INTERP", PyLong_FromLong(ELFProgram::PT_INTERP));
	PyDict_SetItemString(dict, "PT_NOTE", PyLong_FromLong(ELFProgram::PT_NOTE));
	PyDict_SetItemString(dict, "PT_SHLIB", PyLong_FromLong(ELFProgram::PT_SHLIB));
	PyDict_SetItemString(dict, "PT_PHDR", PyLong_FromLong(ELFProgram::PT_PHDR));
	
	ELFProgramType.tp_dict = dict;
	return PyType_Ready(&ELFProgramType) == 0;
}
