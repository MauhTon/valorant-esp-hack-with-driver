#include <windows.h>
#include <D3D11.h>
#include <stdio.h>
#include "Utils.h"
#include "Hook.h"
#include "Settings.h"
#include "Globals.h"

#pragma comment(lib, "d3d11.lib")

void OnDllAttach(PVOID hModule)
{
	// allocate debug console
	AllocConsole();
	freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);

	SetConsoleTitleA(" OverflowR6");
	Utils::Log("Console Allocated!");

	// Inatilizae 
	Globals::HackInit();
	Hooks::HookInit();

	return;
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		OnDllAttach(hModule);
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}


