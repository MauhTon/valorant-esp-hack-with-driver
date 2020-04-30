#include "Engine.h"
#include "Utils.h"
#include "Globals.h"
#include "Vector.hpp"
#include "Vector2D.hpp"

constexpr float r2d = 57.2957795131f;
constexpr float d2r = 0.01745329251f;
#define M_PI		3.14159265358979323846f
#define M_RADPI		57.295779513082f
#define M_PI_F		((float)(M_PI))	// Shouldn't collide with anything.
#define RAD2DEG( x )  ( (float)(x) * (float)(180.f / M_PI_F) )
#define DEG2RAD( x )  ( (float)(x) * (float)(M_PI_F / 180.f) )

class SettingsManager
{
public:
	char pad_0000[72]; //0x0000
	float GPUVendor; //0x0048
	char pad_004C[188]; //0x004C
	int32_t GPUDedicatedMemoryMB; //0x0108
	char pad_010C[36]; //0x010C
	float GPUScore; //0x0130
	char pad_0134[76]; //0x0134
	int32_t SystemMemoryMB; //0x0180
	char pad_0184[36]; //0x0184
	float CPUScore; //0x01A8
	char pad_01AC[52]; //0x01AC
	wchar_t* GPUName; //0x01E0
	char pad_01E8[48]; //0x01E8
	int32_t InitialWindowPosX; //0x0218
	char pad_021C[36]; //0x021C
	int32_t InitialWindowPosY; //0x0240
	char pad_0244[92]; //0x0244
	int32_t ResolutionWidth2; //0x02A0
	char pad_02A4[52]; //0x02A4
	int32_t ResolutionHeight2; //0x02D8
	char pad_02DC[52]; //0x02DC
	float RefreshRate; //0x0310
	char pad_0314[44]; //0x0314
	int32_t WindowedMode0; //0x0340
	char pad_0344[4]; //0x0344
	int32_t WindowedMode; //0x0348
	char pad_034C[268]; //0x034C
	float DefaultFOV0; //0x0458
	char pad_045C[948]; //0x045C
	int32_t ResolutionWidth; //0x0810
	char pad_0814[52]; //0x0814
	int32_t ResolutionHeight; //0x0848
	char pad_084C[52]; //0x084C
	float RefreshRate2; //0x0880
	char pad_0884[164]; //0x0884
	int32_t VSync; //0x0928
	char pad_092C[156]; //0x092C
	float DefaultFOV; //0x09C8
	char pad_09CC[2076]; //0x09CC
	float Brightness; //0x11E8
	char pad_11EC[1340]; //0x11EC
	char* PlayerName; //0x1728
	char* PlayerName2; //0x1730
	char pad_1738[6408]; //0x1738
}; //Size: 0x3040

void C_Engine::SetReolution()
{
	auto pSettings = reinterpret_cast<SettingsManager*>(g_pOffSettings);
	Globals::g_iWindowWidth = pSettings->ResolutionWidth; Globals::g_iWindowHeight = pSettings->ResolutionHeight;
}

uint16_t C_Engine::GetMaxEntitys()
{
	return *(uint16_t*)(g_pOffGame + 0x1D0); // 2bytes
}

Array<C_BaseEntity*> C_Engine::GetEntities()
{
	return *(Array<C_BaseEntity*>*)(g_pOffGame + 0x1C8);
}

C_BaseEntity* C_Engine::GetLocal()
{
	return Utils::ReadPtr<C_BaseEntity*>({ g_pOffProfile, 0x68, 0x0, 0x28 }, false);
}

C_Camera* C_Engine::GetCamera()
{
	return Utils::ReadPtr<C_Camera*>({ g_pOffCamera, 0xE0, 0x1D8, 0x8 }, true); // deref first addy
}

bool C_Engine::IsInGame()
{
	return Utils::Read<bool>(g_pOffStatus + 0x374);
}

bool C_Engine::WorldToScreen(const Vector& origin, Vector2D& screen)
{
	g_pCamera = GetCamera();
	if (!g_pCamera)
		return false;

	Vector temp = origin - g_pCamera->GetViewTranslation();
	float x = temp.Dot(g_pCamera->GetViewRight());
	float y = temp.Dot(g_pCamera->GetViewUp());
	float z = temp.Dot(g_pCamera->GetViewForward() * -1);
	screen.x = (Globals::g_iWindowWidth / 2) * (1 + x / g_pCamera->GetViewFovX() / z);
	screen.y = (Globals::g_iWindowHeight / 2) * (1 - y / g_pCamera->GetViewFovY() / z);

	return z >= 1.0f;
}

float C_Engine::W2SDistance(Vector position)
{
	if (!g_pCamera)
		return -1;

	Vector2D out;
	WorldToScreen(position, out);
	return (fabs(out.x - (Globals::g_iWindowWidth / 2)) + fabs(out.y - (Globals::g_iWindowHeight / 2)));
}

Vector C_Engine::CalcAngle(Vector enemypos, Vector camerapos)
{
	float r2d = 57.2957795131f;

	Vector dir = enemypos - camerapos;

	float x = asin(dir.z / dir.Length()) * r2d;
	float z = atan(dir.y / dir.x) * r2d;

	if (dir.x >= 0.f) z += 180.f;
	if (x > 180.0f) x -= 360.f;
	else if (x < -180.0f) x += 360.f;

	return Vector(x, 0.f, z + 90.f);
}