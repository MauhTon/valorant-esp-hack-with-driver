#include "Utils.h"

namespace g_Settings
{
	// Menu
	bool bMenu = false;
	bool bShutDown = false;
	PVOID hModule = nullptr;


	// Aimbot
	bool bAimbot = false;
	int  iAimbot = false;
	int  iFov = 90;
	int  iBone = 0;

	// Entity
	bool  bChams = true;
	bool  bRecoil = true;
	bool  bSpread = true;
	float  fSpeed = 175.f;
	float  fWepFov = 1.4;
	float  fCharFov = 1.4;
}
