/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifdef USE_PYTHON_DLL

#include "XBPythonDll.h"
#include "cores/DllLoader/DllLoader.h"

// macro to make life a bit simpler

#define DLL_FUNCTION(function) #function, (void**)&p_##function

// use these if the dll exports data structs
#define DLL_OBJECT_DATA(data) #data, (void**)&pointer_##data

#define FUNCTION(function) void* (__cdecl* p_##function)();

extern "C"
{
  /*****************************************
   * python24.dll
   */

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
  FUNCTION(PyObject_CallFunction) // va arg
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
  FUNCTION(PyObject_GetAttrString)
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
  FUNCTION(PyObject_Malloc)    
  FUNCTION(PyObject_Free)   
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
  FUNCTION(PyList_Append)  
  FUNCTION(PyList_SetItem)  
  FUNCTION(PyCObject_AsVoidPtr)  
  FUNCTION(PyOS_snprintf)  
  FUNCTION(PyEval_CallObjectWithKeywords)
  FUNCTION(PySlice_GetIndices)
  FUNCTION(PyThreadState_SetAsyncExc)
  FUNCTION(PySys_SetObject)
  FUNCTION(PySys_GetObject)
  
  DATA_OBJECT(PyExc_SystemError)
  DATA_OBJECT(PyExc_ValueError)
  DATA_OBJECT(PyExc_Exception)
  DATA_OBJECT(PyExc_TypeError)
  DATA_OBJECT(PyExc_KeyboardInterrupt)
  DATA_OBJECT(PyExc_RuntimeError)
  DATA_OBJECT(PyExc_ReferenceError)
  DATA_OBJECT(PyExc_OverflowError)
  DATA_OBJECT(PyExc_NotImplementedError)
  DATA_OBJECT(PyExc_IndexError)
  DATA_OBJECT(PyExc_AttributeError)
  DATA_OBJECT(PyExc_IOError)
  DATA_OBJECT(PyExc_SyntaxError)
  DATA_OBJECT(PyExc_ZeroDivisionError)
  DATA_OBJECT(PyExc_MemoryError)
  DATA_OBJECT(PyExc_StopIteration)
  DATA_OBJECT(PyExc_NameError)

  DATA_OBJECT(PyClass_Type)
  DATA_OBJECT(_PyWeakref_CallableProxyType)
  DATA_OBJECT(PyType_Type)
  DATA_OBJECT(_PyWeakref_ProxyType)
  DATA_OBJECT(PyInstance_Type)
  DATA_OBJECT(PyCFunction_Type)
  
  DATA_OBJECT(_Py_NoneStruct)
  DATA_OBJECT(_Py_NotImplementedStruct)
  DATA_OBJECT(_Py_TrueStruct)
  DATA_OBJECT(_Py_ZeroStruct)
  DATA_OBJECT(PyString_Type)
  DATA_OBJECT(PyList_Type)
  DATA_OBJECT(PyLong_Type)
  DATA_OBJECT(PyInt_Type)
  DATA_OBJECT(PyUnicode_Type)
  DATA_OBJECT(PyTuple_Type)
  DATA_OBJECT(PyDict_Type)
  DATA_OBJECT(PyFloat_Type)
  DATA_OBJECT(PySlice_Type)
  DATA_OBJECT(PyExc_SystemExit)

  bool python_load_dll(LibraryLoader& dll)
  {
    bool bResult;

    bResult = (
      dll.ResolveExport(DLL_FUNCTION(PyEval_ReleaseLock)) &&
      dll.ResolveExport(DLL_FUNCTION(PyEval_AcquireLock)) &&
      dll.ResolveExport(DLL_FUNCTION(PyThreadState_Get)) &&
      dll.ResolveExport(DLL_FUNCTION(PyRun_SimpleString)) &&
      dll.ResolveExport(DLL_FUNCTION(PyRun_String)) &&
      dll.ResolveExport(DLL_FUNCTION(PyImport_AddModule)) &&
      dll.ResolveExport(DLL_FUNCTION(PyEval_InitThreads)) &&
      dll.ResolveExport(DLL_FUNCTION(PyEval_ThreadsInitialized)) &&
      dll.ResolveExport(DLL_FUNCTION(Py_Initialize)) &&
      dll.ResolveExport(DLL_FUNCTION(Py_IsInitialized)) &&
      dll.ResolveExport(DLL_FUNCTION(Py_Finalize)) &&
      dll.ResolveExport(DLL_FUNCTION(Py_NewInterpreter)) &&
      dll.ResolveExport(DLL_FUNCTION(Py_EndInterpreter)) &&
      dll.ResolveExport(DLL_FUNCTION(PyThreadState_Swap)) &&
      dll.ResolveExport(DLL_FUNCTION(PyErr_SetString)) &&
      dll.ResolveExport(DLL_FUNCTION(PyThreadState_New)) &&
      dll.ResolveExport(DLL_FUNCTION(PyErr_Print)) &&
      dll.ResolveExport(DLL_FUNCTION(PyErr_Occurred)) &&
      dll.ResolveExport(DLL_FUNCTION(PyRun_SimpleFile)) &&
      dll.ResolveExport(DLL_FUNCTION(PySys_SetPath)) &&
#ifdef _LINUX
      dll.ResolveExport(DLL_FUNCTION(Py_GetPath)) &&
#endif
      dll.ResolveExport(DLL_FUNCTION(PyThreadState_Delete)) &&
      dll.ResolveExport(DLL_FUNCTION(PyThreadState_Clear)) &&
      dll.ResolveExport(DLL_FUNCTION(Py_BuildValue)) &&
      dll.ResolveExport(DLL_FUNCTION(PyType_IsSubtype)) &&
      dll.ResolveExport(DLL_FUNCTION(PyArg_ParseTupleAndKeywords)) &&
      dll.ResolveExport(DLL_FUNCTION(PyString_AsString)) &&
      dll.ResolveExport(DLL_FUNCTION(Py_AddPendingCall)) &&
      dll.ResolveExport(DLL_FUNCTION(PyObject_CallMethod)) &&
      dll.ResolveExport(DLL_FUNCTION(PyObject_CallMethodObjArgs)) &&
      dll.ResolveExport(DLL_FUNCTION(PyList_GetItem)) &&
      dll.ResolveExport(DLL_FUNCTION(PyList_Size)) &&
      dll.ResolveExport(DLL_FUNCTION(_PyObject_New)) &&
      dll.ResolveExport(DLL_FUNCTION(PyObject_GetAttrString)) &&
      dll.ResolveExport(DLL_FUNCTION(PyLong_AsLong)) &&
      dll.ResolveExport(DLL_FUNCTION(PyLong_AsLongLong)) &&
      dll.ResolveExport(DLL_FUNCTION(PyErr_Format)) &&
#if !defined(_LINUX)
      dll.ResolveExport(DLL_FUNCTION(PyUnicodeUCS2_AsUnicode)) &&
#else
      dll.ResolveExport(DLL_FUNCTION(PyUnicodeUCS4_AsUnicode)) &&
#endif
      dll.ResolveExport(DLL_FUNCTION(Py_MakePendingCalls)) &&
      dll.ResolveExport(DLL_FUNCTION(PyEval_SaveThread)) &&
      dll.ResolveExport(DLL_FUNCTION(PyEval_RestoreThread)) &&
      dll.ResolveExport(DLL_FUNCTION(PyLong_FromLong)) &&
      dll.ResolveExport(DLL_FUNCTION(PyModule_AddStringConstant)) &&
      dll.ResolveExport(DLL_FUNCTION(PyModule_AddObject)) &&
#if (defined USE_EXTERNAL_PYTHON) && (!defined HAVE_LIBPYTHON2_4)
/* Upstream Python rename Py_InitModule4 for 64-bit systems, for Python versions
 higher than 2.4 */
#if SIZEOF_SIZE_T != SIZEOF_INT
      dll.ResolveExport(DLL_FUNCTION(Py_InitModule4_64)) &&
#else
      dll.ResolveExport(DLL_FUNCTION(Py_InitModule4)) &&
#endif
#else
      dll.ResolveExport(DLL_FUNCTION(Py_InitModule4)) &&
#endif
      dll.ResolveExport(DLL_FUNCTION(PyInt_AsLong)) &&
      dll.ResolveExport(DLL_FUNCTION(PyFloat_AsDouble)) &&
      dll.ResolveExport(DLL_FUNCTION(PyString_FromString)) &&
      dll.ResolveExport(DLL_FUNCTION(PyModule_AddIntConstant)) &&
      dll.ResolveExport(DLL_FUNCTION(PyObject_CallFunction)) &&
      dll.ResolveExport(DLL_FUNCTION(PyDict_SetItemString)) &&
      dll.ResolveExport(DLL_FUNCTION(PyDict_New)) &&
      dll.ResolveExport(DLL_FUNCTION(PyModule_GetDict)) &&
      dll.ResolveExport(DLL_FUNCTION(PyImport_Import)) &&
      dll.ResolveExport(DLL_FUNCTION(PyFloat_FromDouble)) &&
      dll.ResolveExport(DLL_FUNCTION(PyInt_FromLong)) &&
      dll.ResolveExport(DLL_FUNCTION(PyDict_GetItemString)) &&
      //dll.ResolveExport(DLL_FUNCTION(PyDict_GetItem)) &&
      //dll.ResolveExport(DLL_FUNCTION(PyDict_Keys)) &&
      dll.ResolveExport(DLL_FUNCTION(PyDict_Next)) &&
      dll.ResolveExport(DLL_FUNCTION(PyDict_Size)) &&
      dll.ResolveExport(DLL_FUNCTION(PyType_Ready)) &&
      dll.ResolveExport(DLL_FUNCTION(PyType_GenericNew)) &&
      dll.ResolveExport(DLL_FUNCTION(PyArg_Parse)) &&
      dll.ResolveExport(DLL_FUNCTION(PyArg_ParseTuple)) &&
      dll.ResolveExport(DLL_FUNCTION(PySys_SetArgv)) &&
      dll.ResolveExport(DLL_FUNCTION(PyObject_RichCompare)) &&
      dll.ResolveExport(DLL_FUNCTION(PyObject_GetAttrString)) &&
      dll.ResolveExport(DLL_FUNCTION(_PyObject_GetDictPtr)) &&
      dll.ResolveExport(DLL_FUNCTION(PyTuple_SetItem)) &&
      dll.ResolveExport(DLL_FUNCTION(PyDict_SetItem)) &&
      dll.ResolveExport(DLL_FUNCTION(PyErr_Clear)) &&
      dll.ResolveExport(DLL_FUNCTION(PyDict_GetItem)) &&
      dll.ResolveExport(DLL_FUNCTION(PyString_FromFormat)) &&
      dll.ResolveExport(DLL_FUNCTION(PyObject_GetAttr)) &&
      dll.ResolveExport(DLL_FUNCTION(PyObject_Call)) &&
      dll.ResolveExport(DLL_FUNCTION(PyObject_Init)) &&
      dll.ResolveExport(DLL_FUNCTION(PyObject_Size)) &&
      dll.ResolveExport(DLL_FUNCTION(PyString_Format)) &&
      dll.ResolveExport(DLL_FUNCTION(PyTuple_New)) &&
      dll.ResolveExport(DLL_FUNCTION(PyString_ConcatAndDel)) &&
      dll.ResolveExport(DLL_FUNCTION(PyCObject_Import)) &&
      dll.ResolveExport(DLL_FUNCTION(PyObject_IsTrue)) &&
      dll.ResolveExport(DLL_FUNCTION(PyObject_GenericGetAttr)) &&
      dll.ResolveExport(DLL_FUNCTION(PyInstance_NewRaw)) &&
      dll.ResolveExport(DLL_FUNCTION(_PyInstance_Lookup)) &&
      dll.ResolveExport(DLL_FUNCTION(PyBool_FromLong)) &&
      dll.ResolveExport(DLL_FUNCTION(PyCObject_FromVoidPtr)) &&
      dll.ResolveExport(DLL_FUNCTION(PyLong_FromVoidPtr)) &&
      dll.ResolveExport(DLL_FUNCTION(PyArg_UnpackTuple)) &&
      dll.ResolveExport(DLL_FUNCTION(PyObject_CallFunctionObjArgs)) &&
      dll.ResolveExport(DLL_FUNCTION(PyObject_Free)) &&
      dll.ResolveExport(DLL_FUNCTION(PyObject_Malloc)) &&
      dll.ResolveExport(DLL_FUNCTION(PyString_FromStringAndSize)) &&
      dll.ResolveExport(DLL_FUNCTION(PyString_AsStringAndSize)) &&
      dll.ResolveExport(DLL_FUNCTION(PyObject_SetAttr)) &&
      dll.ResolveExport(DLL_FUNCTION(PyGILState_Ensure)) &&
      dll.ResolveExport(DLL_FUNCTION(PyGILState_Release)) &&
      dll.ResolveExport(DLL_FUNCTION(PyErr_SetObject)) &&
      dll.ResolveExport(DLL_FUNCTION(PyLong_AsUnsignedLong)) &&
      dll.ResolveExport(DLL_FUNCTION(PySequence_Size)) &&
      dll.ResolveExport(DLL_FUNCTION(PyErr_Fetch)) &&
      dll.ResolveExport(DLL_FUNCTION(PySequence_Check)) &&
      dll.ResolveExport(DLL_FUNCTION(PySequence_GetItem)) &&
      dll.ResolveExport(DLL_FUNCTION(PyObject_Str)) &&
      dll.ResolveExport(DLL_FUNCTION(PyLong_FromUnsignedLong)) &&
      dll.ResolveExport(DLL_FUNCTION(PyList_New)) &&
      dll.ResolveExport(DLL_FUNCTION(PyLong_AsDouble)) &&
      dll.ResolveExport(DLL_FUNCTION(PyList_Append)) &&
      dll.ResolveExport(DLL_FUNCTION(PyOS_snprintf)) &&
      dll.ResolveExport(DLL_FUNCTION(PyCObject_AsVoidPtr)) &&
      dll.ResolveExport(DLL_FUNCTION(PyList_SetItem)) &&
      dll.ResolveExport(DLL_FUNCTION(PyEval_CallObjectWithKeywords)) &&
      dll.ResolveExport(DLL_FUNCTION(PyEval_CallObjectWithKeywords)) &&
      dll.ResolveExport(DLL_FUNCTION(PySlice_GetIndices)) &&
      dll.ResolveExport(DLL_FUNCTION(PySys_SetObject)) &&
      dll.ResolveExport(DLL_FUNCTION(PySys_GetObject)) &&
      dll.ResolveExport(DLL_FUNCTION(PyThreadState_SetAsyncExc)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyExc_SystemError)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyExc_ValueError)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyExc_Exception)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyExc_TypeError)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyExc_KeyboardInterrupt)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyExc_RuntimeError)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyExc_ReferenceError)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyExc_OverflowError)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyExc_NotImplementedError)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyExc_IndexError)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyExc_AttributeError)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyExc_IOError)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyExc_SyntaxError)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyExc_ZeroDivisionError)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyExc_MemoryError)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyExc_StopIteration)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyExc_NameError)) &&

      dll.ResolveExport(DLL_OBJECT_DATA(PyClass_Type)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(_PyWeakref_CallableProxyType)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyType_Type)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(_PyWeakref_ProxyType)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyInstance_Type)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyCFunction_Type)) &&
      
      dll.ResolveExport(DLL_OBJECT_DATA(_Py_NoneStruct)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(_Py_NotImplementedStruct)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(_Py_TrueStruct)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(_Py_ZeroStruct)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyString_Type)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyList_Type)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyLong_Type)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyInt_Type)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyUnicode_Type)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyTuple_Type)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyFloat_Type)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PySlice_Type)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyDict_Type)) &&
      dll.ResolveExport(DLL_OBJECT_DATA(PyExc_SystemExit)));

    return bResult;
  }
}

#endif

