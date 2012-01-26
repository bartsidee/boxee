#ifdef USE_PYTHON_DLL

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif

#if defined(__arm__)
#define FUNCTION(name)                  \
         .globl name;                \
         name:                       \
             ldr r12,=p_##name; ldr r12, [r12]; bx r12
#else
#ifndef __APPLE__
#if defined(__PPC__)
#define FUNCTION(name)			\
	.globl name;			\
	.type name, @function;		\
	name:				\
		lis 11,(p_##name)@ha;		\
		lwz 12,(p_##name)@l(11);	\
		mtctr 12;			\
		bctr 
#else
#define FUNCTION(name)			\
	.globl name;			\
	.type name, @function;		\
	name:				\
		jmp *(p_##name)
#endif
#elif defined(__POWERPC__)
#define FUNCTION(name)				\
	.globl _##name				@\
	_##name:				@\
		addis r11,0,ha16(_p_##name)	@\
		lwz r12,lo16(_p_##name)(r11)	@\
		mtctr r12			@\
		bctr 
#else
#define FUNCTION(name)			\
	.globl _##name;			\
	_##name:				\
		jmp *(_p_##name)
#endif
#endif

.text

  FUNCTION(PyEval_ReleaseLock)
  FUNCTION(PyEval_AcquireLock)
  FUNCTION(PyThreadState_Get)
  FUNCTION(PyRun_SimpleString)
  FUNCTION(PyRun_String)
  FUNCTION(PyImport_AddModule)
  FUNCTION(PyEval_InitThreads)
  FUNCTION(PyEval_ThreadsInitialized)
  FUNCTION(Py_Initialize)
  FUNCTION(Py_IsInitialized)
  FUNCTION(Py_Finalize)
  FUNCTION(Py_NewInterpreter)
  FUNCTION(Py_EndInterpreter)
  FUNCTION(PyThreadState_Swap)
  FUNCTION(PyErr_SetString)
  FUNCTION(PyThreadState_New)
  FUNCTION(PyErr_Print)
  FUNCTION(PyErr_Occurred)
  FUNCTION(PyRun_SimpleFile)
  FUNCTION(PySys_SetPath)
#ifdef _LINUX
  FUNCTION(Py_GetPath)
#endif
  FUNCTION(PyThreadState_Delete)
  FUNCTION(PyThreadState_Clear)
  FUNCTION(Py_BuildValue)
  FUNCTION(PyArg_Parse)
  FUNCTION(PyArg_ParseTuple)
  FUNCTION(PyType_IsSubtype)
  FUNCTION(PyArg_ParseTupleAndKeywords)
  FUNCTION(PyString_AsString)
  FUNCTION(Py_AddPendingCall)
  FUNCTION(PyList_GetItem)
  FUNCTION(PyList_Size)
  FUNCTION(_PyObject_New)
  FUNCTION(PyObject_GetAttrString)
  FUNCTION(PyLong_AsLong)
  FUNCTION(PyLong_AsLongLong)
  FUNCTION(PyErr_Format)
#if !defined(_LINUX)
  FUNCTION(PyUnicodeUCS2_AsUnicode)
#else
  FUNCTION(PyUnicodeUCS4_AsUnicode)
#endif
  FUNCTION(Py_MakePendingCalls)
  FUNCTION(PyEval_SaveThread)
  FUNCTION(PyEval_RestoreThread)
  FUNCTION(PyLong_FromLong)
  FUNCTION(PyModule_AddStringConstant)
  FUNCTION(PyModule_AddObject)
#if (defined USE_EXTERNAL_PYTHON) && (!defined HAVE_LIBPYTHON2_4)
  /* Upstream Python rename Py_InitModule4 for 64-bit systems, for Python
   versions higher than 2.4 */
  #if SIZEOF_SIZE_T != SIZEOF_INT 
  FUNCTION(Py_InitModule4_64)
  #else
  FUNCTION(Py_InitModule4)
  #endif
#else
  FUNCTION(Py_InitModule4)
#endif
  FUNCTION(PyInt_AsLong)
  FUNCTION(PyFloat_AsDouble)
  FUNCTION(PyString_FromString)
  FUNCTION(PyModule_AddIntConstant)
  FUNCTION(PyObject_CallFunction)
  FUNCTION(PyObject_CallMethod)
  FUNCTION(PyObject_CallMethodObjArgs)
  FUNCTION(PyDict_SetItemString)
  FUNCTION(PyDict_New)
  FUNCTION(PyModule_GetDict)
  FUNCTION(PyImport_Import)
  FUNCTION(PyInt_FromLong)
  FUNCTION(PyDict_GetItemString)
  FUNCTION(PyDict_Next)
  FUNCTION(PyDict_Size)
  FUNCTION(PyType_Ready)
  FUNCTION(PyType_GenericNew)
  FUNCTION(PySys_SetArgv)
  FUNCTION(PyObject_RichCompare)
  FUNCTION(PyFloat_FromDouble)
  FUNCTION(PyObject_Free)
  FUNCTION(PyObject_Malloc)
  FUNCTION(PyString_FromStringAndSize)
  FUNCTION(PyString_AsStringAndSize)
  FUNCTION(PyObject_SetAttr)
  FUNCTION(PyGILState_Ensure)
  FUNCTION(PyGILState_Release)
  FUNCTION(PyErr_SetObject)
  FUNCTION(PyLong_AsUnsignedLong)
  FUNCTION(PySequence_Size)
  FUNCTION(PyErr_Fetch)
  FUNCTION(PySequence_Check)
  FUNCTION(PySequence_GetItem)
  FUNCTION(PyObject_Str)
  FUNCTION(PyLong_FromUnsignedLong)
  FUNCTION(PyList_New)  
  FUNCTION(PyLong_AsDouble)
  FUNCTION(PyOS_snprintf)
  FUNCTION(PyCObject_AsVoidPtr)
  FUNCTION(PyList_SetItem)
  FUNCTION(PyList_Append)
  
  FUNCTION(_PyObject_GetDictPtr)
  FUNCTION(PyTuple_SetItem)
  FUNCTION(PyDict_SetItem)
  FUNCTION(PyErr_Clear)
  FUNCTION(PyDict_GetItem)
  FUNCTION(PyString_FromFormat)
  FUNCTION(PyObject_GetAttr)
  FUNCTION(PyObject_Call)
  FUNCTION(PyObject_Init)
  FUNCTION(PyObject_Size)
  FUNCTION(PyString_Format)
  FUNCTION(PyTuple_New)
  FUNCTION(PyString_ConcatAndDel)
  FUNCTION(PyCObject_Import)
  FUNCTION(PyObject_IsTrue)
  FUNCTION(PyObject_GenericGetAttr)
  FUNCTION(PyInstance_NewRaw)
  FUNCTION(_PyInstance_Lookup)
  FUNCTION(PyBool_FromLong)
  FUNCTION(PyCObject_FromVoidPtr)
  FUNCTION(PyLong_FromVoidPtr)
  FUNCTION(PyArg_UnpackTuple)
  FUNCTION(PyObject_CallFunctionObjArgs)
  FUNCTION(PyEval_CallObjectWithKeywords)
  FUNCTION(PySlice_GetIndices)
  FUNCTION(PySys_SetObject)
  FUNCTION(PySys_GetObject)
  FUNCTION(PyThreadState_SetAsyncExc)
  
#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif

#endif

