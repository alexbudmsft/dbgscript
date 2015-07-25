#include "../include/iscriptprovider.h"
#include "pythonscriptprovider.h"
// Later: #include "rubyscriptprovider.h"
//

// TODO: Every provider will expose a factor method to create itself.
// All installed providers will be created on DLL load. Appropriate one is then
// invoked based on file extension claims.
//
IScriptProvider*
CreateScriptProvider()
{
	IScriptProvider* prov = new CPythonScriptProvider();

	return prov;
}

