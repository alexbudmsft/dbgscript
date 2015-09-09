#include "util.h"
#include "common.h"
#include <python.h>

// Attribute is read-only. TODO: Share this out to other modules.
//
int
SetReadOnlyProperty(
	PyObject* /* self */, PyObject* /* value */, void* /* closure */)
{
	PyErr_SetString(PyExc_AttributeError, "readonly attribute");
	return -1;
}

// Replace sys.stdout and sys.stderr with 'obj'.
//
void
RedirectStdIO(
	_In_ PyObject* obj)
{
	PySys_SetObject("stdout", obj);
	PySys_SetObject("__stdout__", obj);
	PySys_SetObject("stderr", obj);
	PySys_SetObject("__stderr__", obj);
	PySys_SetObject("stdin", obj);
	PySys_SetObject("__stdin__", obj);
}

//------------------------------------------------------------------------------
// Function: PyReadBytes
//
// Description:
//
//  Read 'count' bytes from 'addr'
//  
// Returns:
//
// Notes:
//
PyObject*
PyReadBytes(
	_In_ UINT64 addr,
	_In_ ULONG count)
{
	char* buf = nullptr;
	ULONG cbActual = 0;
	PyObject* ret = nullptr;
	
	buf = new char[count];
	if (!buf)
	{
		PyErr_SetString(PyExc_MemoryError, "Couldn't allocate buffer.");
		goto exit;
	}
	
	HRESULT hr = UtilReadBytes(
		GetPythonProvGlobals()->HostCtxt, addr, buf, count, &cbActual);
	if (FAILED(hr))
	{
		PyErr_Format(PyExc_RuntimeError, "UtilReadBytes failed. Error 0x%08x.", hr);
		goto exit;
	}
	
	ret = PyBytes_FromStringAndSize(buf, cbActual);
	
exit:
	// Python makes an internal copy of the input buffer.
	//
	delete [] buf;
	
	return ret;
}


