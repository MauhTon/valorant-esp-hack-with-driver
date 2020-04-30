#include "Utils.h"
#include "Features.h"
#include "Globals.h"
#include "Settings.h"

void Features::RenderESP(D3D11Renderer* Render, nk_context* g_pNkContext)
{
	Array<C_BaseEntity*> Entitylist = g_pEngine->GetEntities();
	for (uint32_t i = 0; i < g_pEngine->GetMaxEntitys(); i++)
	{
		auto pEntity = Entitylist[i];

		if (!pEntity)
			continue;

		if (pEntity == g_pLocalEntity)
			continue;

		if (!pEntity->IsAlive())
			continue;

		if (pEntity->GetTeamNumber() == g_pLocalEntity->GetTeamNumber())
			continue;

		Vector2D vecScreenOrgin, vecScreenHead;
		Vector orgin = pEntity->GetFeet(), head = pEntity->GetHead();
		if (g_pEngine->WorldToScreen(orgin, vecScreenOrgin) && g_pEngine->WorldToScreen(head, vecScreenHead))
		{
			int iMiddle = (vecScreenOrgin.y - vecScreenHead.y);
			int iWidth = iMiddle / 4;
			
			Render->DrawBox(vecScreenHead.x - iWidth, vecScreenHead.y, iWidth * 2, iMiddle, Color{255, 255, 255, 0});
			Render->DrawBox(vecScreenHead.x - iWidth - 1, vecScreenHead.y - 1, (iWidth * 2) + 2, iMiddle + 2, Color{255, 0, 0, 0});
			Render->DrawHealthBar(vecScreenHead.x - iWidth - 6, vecScreenHead.y, iWidth / 5.5f, iMiddle, pEntity->GetHealth(), Color{ 255, 255, 255, 0 });
			//Render->DrawString(g_pNkContext, pEntity->GetPlayerName().c_str(), vecScreenHead.x, vecScreenHead.y - 6, Color{ 255, 255, 255, 0 });
			Render->DrawCircle(vecScreenHead.x, vecScreenHead.y, (vecScreenOrgin.y - vecScreenHead.y) / 10, 30, Color{ 255, 255, 255, 0 });
			Render->DrawLine(Globals::g_iWindowWidth / 2, Globals::g_iWindowHeight, vecScreenOrgin.x, vecScreenOrgin.y, Color{ 255, 255, 255, 0 });
		}

		Render->DrawCircle(Globals::g_iWindowWidth / 2, Globals::g_iWindowHeight / 2, g_Settings::iFov, 30, Color{ 255, 255, 255, 0 });
	}
}