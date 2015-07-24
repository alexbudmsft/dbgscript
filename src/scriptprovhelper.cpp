#include "../include/iscriptprovider.h"
#include "pythonscriptprovider.h"
// Later: #include "rubyscriptprovider.h"
//

IScriptProvider*
CreateScriptProvider(
	_In_z_ const WCHAR* scriptName)
{
	// TODO: check file extension.
	//

	IScriptProvider* prov = new CPythonScriptProvider(scriptName);

	return prov;
}

