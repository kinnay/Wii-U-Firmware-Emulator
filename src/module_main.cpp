
#include <Python.h>
#include "type_elffile.h"
#include "type_elfprogram.h"
#include "type_physmem.h"
#include "type_ivirtmem.h"
#include "type_armmmu.h"
#include "type_armcore.h"
#include "type_arminterp.h"
#include "type_ppclock.h"
#include "type_ppcmmu.h"
#include "type_ppccore.h"
#include "type_ppcinterp.h"
#include "type_scheduler.h"
#include "type_sha1.h"
#include "errors.h"

#define ADD(name, type) \
	Py_INCREF(type); \
	PyModule_AddObject(m, name, (PyObject *)type);
	
#define ERROR(name) \
	extern PyObject *MyExc_ ## name;

ERROR(ParseError);

PyModuleDef MainModule = {
	PyModuleDef_HEAD_INIT,
	"pyemu",
	"PyEmu Docstring",
	-1
};

PyMODINIT_FUNC PyInit_pyemu() {
	Errors_Init();
	if (!ELFFileType_Init()) return NULL;
	if (!ELFProgramType_Init()) return NULL;
	if (!PhysMemType_Init()) return NULL;
	if (!IVirtMemType_Init()) return NULL;
	if (!InterpreterType_Init()) return NULL;
	if (!ARMMMUType_Init()) return NULL;
	if (!ARMCoreType_Init()) return NULL;
	if (!ARMInterpType_Init()) return NULL;
	if (!PPCLockType_Init()) return NULL;
	if (!PPCMMUType_Init()) return NULL;
	if (!PPCCoreType_Init()) return NULL;
	if (!PPCInterpType_Init()) return NULL;
	if (!SchedulerType_Init()) return NULL;
	if (!SHA1Type_Init()) return NULL;
	
	PyObject *m = PyModule_Create(&MainModule);
	if (!m) return NULL;
	
	ADD("ParseError", MyExc_ParseError);
	ADD("ELFFile", &ELFFileType);
	ADD("ELFProgram", &ELFProgramType);
	ADD("PhysicalMemory", &PhysMemType);
	ADD("IVirtualMemory", &IVirtMemType);
	ADD("Interpreter", &InterpreterType);
	ADD("ARMMMU", &ARMMMUType);
	ADD("ARMCore", &ARMCoreType);
	ADD("ARMInterpreter", &ARMInterpType);
	ADD("PPCLockMgr", &PPCLockType);
	ADD("PPCMMU", &PPCMMUType);
	ADD("PPCCore", &PPCCoreType);
	ADD("PPCInterpreter", &PPCInterpType);
	ADD("Scheduler", &SchedulerType);
	ADD("SHA1", &SHA1Type);
	return m;
}
