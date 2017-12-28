
#pragma once

#define CHECK_NOT_INIT(obj) \
	if (obj) { \
		PyErr_SetString(PyExc_RuntimeError, "__init__ may only be called once"); \
		return -1; \
	}
	
#define CHECK_INIT(obj) \
	if (!obj) { \
		PyErr_SetString(PyExc_RuntimeError, "__init__ must be called"); \
		return NULL; \
	}
