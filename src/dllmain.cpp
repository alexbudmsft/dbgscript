#pragma warning(push)
#pragma warning(disable: 4510 4512 4610 4100)
#include <ruby.h>
#pragma warning(pop)

#include <dbgeng.h>

#define DLLEXPORT extern "C" __declspec(dllexport)

BOOL WINAPI DllMain(
	_In_ HINSTANCE /* hinstDLL */,
	_In_ DWORD     fdwReason,
	_In_ LPVOID    /* lpvReserved */)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_PROCESS_DETACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;  // Nothing to do.
	default:
		break;
	}

	return TRUE;
}

DLLEXPORT HRESULT DebugExtensionInitialize(
	_Out_ PULONG Version,
	_Out_ PULONG Flags)
{
	*Version = DEBUG_EXTENSION_VERSION(1, 0);
	*Flags = 0;

	return S_OK;
}

DLLEXPORT HRESULT CALLBACK foo(
	_In_     IDebugClient* client,
	_In_opt_ PCSTR         args)
{
	IDebugControl *dbgCtrl = nullptr;
	HRESULT hr = client->QueryInterface(__uuidof(IDebugControl), (void**)&dbgCtrl);
	if (FAILED(hr))
	{
		goto exit;
	}

	hr = dbgCtrl->Output(DEBUG_OUTPUT_NORMAL, "%s\n", args);
	if (FAILED(hr))
	{
		goto exit;
	}

exit:
	return hr;
}
