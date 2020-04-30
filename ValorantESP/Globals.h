#pragma once
#include "Engine.h"
#include "Entity.h"

extern  C_BaseEntity* g_pLocalEntity;
extern  C_Engine* g_pEngine;
extern  C_Camera* g_pCamera;

extern uintptr_t	g_pOffCamera;
extern uintptr_t   	g_pOffStatus;
extern uintptr_t   	g_pOffGame;
extern uintptr_t   	g_pOffProfile;
extern uintptr_t   	g_pOffFOV;
extern uintptr_t   	g_pOffChams;
extern uintptr_t   	g_pOffSettings;

namespace Globals
{
	extern uintptr_t Base;
	void HackInit();
	extern int g_iWindowWidth;
	extern int g_iWindowHeight;

	extern bool PressedKeys[256];
}