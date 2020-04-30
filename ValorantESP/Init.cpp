#include "Utils.h"
#include "Globals.h"
#include "SDKMisc.h"
#include "C_GameMan.h"
#include "Entity.h"
#include "Engine.h"
#include "Camera.h"

C_Engine*       g_pEngine;
C_BaseEntity*	g_pLocalEntity = nullptr;
C_Camera*		g_pCamera = nullptr;

uintptr_t		g_pOffCamera;
uintptr_t   	g_pOffStatus;
uintptr_t   	g_pOffGame;
uintptr_t   	g_pOffProfile;
uintptr_t   	g_pOffFOV;
uintptr_t   	g_pOffChams;
uintptr_t   	g_pOffSettings;

namespace Globals
{
	uintptr_t Base = NULL;
	void Globals::HackInit()
	{
		Base = reinterpret_cast<uintptr_t>(GetModuleHandleA(NULL));

		// find class offset pointers
		uintptr_t pOff = (uintptr_t)Utils::FindSignature(Base, "48 8b 05 ?? ?? ?? ?? 48 87 38");
		g_pOffCamera = *reinterpret_cast<uintptr_t*>(pOff + *(uint32_t*)(pOff + 3) + 7);
		std::cout << "g_pOffCamera: " << std::hex << g_pOffCamera << std::endl;

		pOff = reinterpret_cast<uintptr_t>(Utils::FindSignature(Base, "48 8b 0d ?? ?? ?? ?? e8 ?? ?? ?? ?? 49 8b 8e ?? ?? ?? ?? E8"));
		g_pOffStatus = *reinterpret_cast<uintptr_t*>(pOff + *(uint32_t*)(pOff + 3) + 7);
		std::cout << "g_pOffStatus: " << std::hex << g_pOffStatus << std::endl;

		pOff = reinterpret_cast<uintptr_t>(Utils::FindSignature(Base, "a0 ?? ?? ?? ?? ?? ?? ?? ?? 03 00 40 00 00 00 00 44 78"));
		std::cout << "pOff: " << std::hex << pOff << std::endl;
		g_pOffGame = *reinterpret_cast<uintptr_t*>(pOff + *(uint32_t*)(pOff + 3) + 7);		
		std::cout << "g_pOffGame: " << std::hex << g_pOffGame << std::endl;

		pOff = reinterpret_cast<uintptr_t>(Utils::FindSignature(Base, "48 8b 05 ?? ?? ?? ?? 45 8b 9A"));
		g_pOffProfile = *reinterpret_cast<uintptr_t*>(pOff + *(uint32_t*)(pOff + 3) + 7);		
		std::cout << "g_pOffProfile: " << std::hex << g_pOffProfile << std::endl;

		pOff = reinterpret_cast<uintptr_t>(Utils::FindSignature(Base, "48 8b 05 ?? ?? ?? ?? f3 44 0f 10 91"));
		g_pOffFOV = *reinterpret_cast<uintptr_t*>(pOff + *(uint32_t*)(pOff + 3) + 7);
		std::cout << "g_pOffFOV: " << std::hex << g_pOffFOV << std::endl;

		pOff = reinterpret_cast<uintptr_t>(Utils::FindSignature(Base, "48 8b 0d ?? ?? ?? ?? 48 8b d7 e8 ?? ?? ?? ?? 48 85 c0 74 ?? 4c"));
		g_pOffChams = *reinterpret_cast<uintptr_t*>(pOff + *(uint32_t*)(pOff + 3) + 7);	
		std::cout << "g_pOffChams: " << std::hex << g_pOffChams << std::endl;

		pOff = reinterpret_cast<uintptr_t>(Utils::FindSignature(Base, "4C 8B 05 ?? ?? ?? ?? 41 8B 80 ?? ?? ?? ?? 48 69"));
		g_pOffSettings = *reinterpret_cast<uintptr_t*>(pOff + *(uint32_t*)(pOff + 3) + 7);
		std::cout << "g_pOffSettings: " << std::hex << g_pOffSettings << std::endl;

		pOff = reinterpret_cast<uintptr_t>(Utils::FindSignature(Base, "c6 ?? 38 ?? 48 85 db 74"));
		std::cout << "g_pOffUnlock+3: " << std::hex << pOff+3 << std::endl;		

		pOff = reinterpret_cast<uintptr_t>(Utils::FindSignature(Base, "83 F8 01 0f 85 ? ? ? ? F3 0F 10 1D"));
		std::cout << "No Recoil: " << std::hex << pOff << std::endl;

		DWORD Old;
		VirtualProtect((LPVOID)TerminateProcess, sizeof(byte), PAGE_EXECUTE_READWRITE, &Old);
		*(byte*)(TerminateProcess) = 0xC3;
		VirtualProtect((LPVOID)TerminateProcess, sizeof(byte), Old, &Old);

		VirtualProtect((LPVOID)pOff, 4, PAGE_EXECUTE_READWRITE, &Old);
		Utils::Write<bool>(pOff + 3, 0);
		VirtualProtect((LPVOID)pOff, 4, Old, &Old);
		
		// Assign class pointers
		g_pLocalEntity = g_pEngine->GetLocal();
		g_pCamera	   = g_pEngine->GetCamera();

		// Call settings functions
		g_pEngine->SetReolution();
	}

	int g_iWindowWidth = 1920;
	int g_iWindowHeight = 1080;
	bool PressedKeys[256];
}
