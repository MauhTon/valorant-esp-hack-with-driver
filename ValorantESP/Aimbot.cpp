#include "Utils.h"
#include "Engine.h"
#include "Globals.h"
#include "Settings.h"
#include "Features.h"
using namespace std;

int g_iBoneID = 0;
C_BaseEntity* FindTargetFOV(int BoneID)
{
	float m_fBestFOV = g_Settings::iFov;
	C_BaseEntity* m_pBestEntity = NULL;
	Array<C_BaseEntity*> Entitylist = g_pEngine->GetEntities();
	for (int i = 0; i < g_pEngine->GetMaxEntitys(); i++)
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

		float pEnemyFov = g_pEngine->W2SDistance(pEntity->GetBonePostionByID(BoneID));
		if (!pEnemyFov)
			continue;

		if (pEnemyFov < m_fBestFOV)
		{
			m_fBestFOV = pEnemyFov;
			m_pBestEntity = pEntity;
		}
	}

	g_iBoneID = BoneID;
	return m_pBestEntity;
}

bool Features::DoAimbot()
{
	C_BaseEntity* pEntity = NULL;
	if (g_Settings::iBone == 0)
		pEntity = FindTargetFOV(BONE_HEAD);
	else if (g_Settings::iBone == 1)
		pEntity = FindTargetFOV(BONE_CHEST);
	else
		pEntity = NULL;

	if (!pEntity)
		return false;

	Vector vecEnemyAngles = g_pEngine->CalcAngle(pEntity->GetBonePostionByID(g_iBoneID), g_pCamera->GetViewTranslation());
	vecEnemyAngles.Clamp();
	g_pLocalEntity->SetViewAngle(vecEnemyAngles);
}