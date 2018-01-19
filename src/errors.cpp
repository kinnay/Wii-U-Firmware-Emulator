
#include <Python.h>

#define ERROR(name) \
	void name(const char *message, ...) { \
		va_list args; \
		va_start(args, message); \
		PyErr_FormatV(PyExc_ ## name, message, args); \
		va_end(args); \
	}
		
#define CUSTOM(name) \
	PyObject *MyExc_ ## name; \
	void name(const char *message, ...) { \
		va_list args; \
		va_start(args, message); \
		PyErr_FormatV(MyExc_ ## name, message, args); \
		va_end(args); \
	}

ERROR(IndexError)
ERROR(IOError)
ERROR(KeyError)
ERROR(MemoryError)
ERROR(NotImplementedError)
ERROR(OverflowError)
ERROR(RuntimeError)
ERROR(TypeError)
ERROR(ValueError)

CUSTOM(ParseError)

bool ErrorOccurred() {
	return PyErr_Occurred();
}

void ErrorClear() {
	PyErr_Clear();
}

void Errors_Init() {
	MyExc_ParseError = PyErr_NewException("pyemu.ParseError", NULL, NULL);
}
