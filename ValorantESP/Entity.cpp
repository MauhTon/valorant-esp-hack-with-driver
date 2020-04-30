#include "Entity.h"
#include "Utils.h"
#include "Settings.h"
#include <string.h>
#include "Vector.hpp"
#include "Engine.h"
#include "Globals.h"

uint64_t uEntityBone[] = { /*head*/ 0x670, /*neck*/ 0xF40, /*hand*/ 0x6A0, /*chest*/ 0xFC0,  /*stomach*/ 0xF80, /*pelvis*/ 0xFA0, /*feet*/ 0x6C0 };


std::string RPMString(DWORD64 address) 
{
	if (!(void*)address)
		return std::string("BOT");

	std::string nameString;
	char name[30];
	memcpy(name, (void*)address, 30);
	for (int i = 0; i < sizeof(name); i++) {
		if (!name[i])
			break;
		if ((int)name[i] >= 32 && (int)name[i] <= 126)
			nameString += name[i];
		else
			break;
	}
	return nameString;

}

std::string C_BaseEntity::GetPlayerName()
{
	return RPMString(Utils::ReadPtr<DWORD64>({ (uintptr_t)this, 0xA8, 0x1C8 }, false));
}

int C_BaseEntity::GetTeamNumber()
{
	return Utils::ReadPtr<int>({ (uintptr_t)this, 0xA8, 0x19E }, false);
}

bool C_BaseEntity::IsAlive()
{
	return GetHealth() > 0 ? true : false;
}

int C_BaseEntity::GetHealth()
{
	return Utils::ReadPtr<int>({ (uintptr_t)this, 0x28, 0xD8, 0x8, 0x148 }, false);
}

Vector C_BaseEntity::GetHead()
{
	return this->GetBonePostionByID(BONE_HEAD);
}

Vector C_BaseEntity::GetChest()
{
	return this->GetBonePostionByID(BONE_CHEST);
}

Vector C_BaseEntity::GetFeet()
{
	return this->GetBonePostionByID(BONE_FEET);
}

Vector4D C_BaseEntity::GetViewAngle()
{
	return Utils::ReadPtr<Vector4D>({ (uintptr_t)this, 0x20, 0x1170, 0xC0 }, false);
}

Vector4D CreateFromYawPitchRoll(float yaw, float pitch, float roll)
{
	Vector4D result;
	float cy = cos(yaw * 0.5);
	float sy = sin(yaw * 0.5);
	float cr = cos(roll * 0.5);
	float sr = sin(roll * 0.5);
	float cp = cos(pitch * 0.5);
	float sp = sin(pitch * 0.5);

	result.w = cy * cr * cp + sy * sr * sp;
	result.x = cy * sr * cp - sy * cr * sp;
	result.y = cy * cr * sp + sy * sr * cp;
	result.z = sy * cr * cp - cy * sr * sp;

	return result;
}

void C_BaseEntity::SetViewAngle(Vector& angle)
{
	float d2r = 0.01745329251f;
	Vector4D vecNewAngle = CreateFromYawPitchRoll(angle.z * d2r, 0.f, angle.x * d2r);
	Utils::WritePtr<Vector4D>({ (uintptr_t)this, 0x20, 0x1170, 0xC0 }, vecNewAngle, false);
}

Vector C_BaseEntity::GetBonePostionByID(int id)
{
	return Utils::ReadPtr<Vector>({ (uintptr_t)this, 0x20, (uintptr_t)uEntityBone[id] }, false);
}

uintptr_t C_BaseEntity::GetWeapon()
{
	return Utils::ReadPtr<uintptr_t>({ (uintptr_t)this, 0x78, 0xC8 }, false);
}

void C_BaseEntity::SetSpeed()
{
	Utils::WritePtr<int>({(uintptr_t)this, 0x30, 0x30, 0x38, 0x58 }, g_Settings::fSpeed, false);
}

void C_BaseEntity::NoRecoil()
{
	auto Weapon = this->GetWeapon();
	if (Weapon)
		Utils::WritePtr<float>({(uintptr_t)Weapon, 0x208, 0xC0 }, .99f, false);
}

void C_BaseEntity::NoSpread()
{
	auto Weapon = this->GetWeapon();
	if (Weapon)
		Utils::WritePtr<float>({ (uintptr_t)Weapon, 0x208, 0x50 }, 0.00001f, false);
}

void C_BaseEntity::NoReload()
{
	auto Weapon = this->GetWeapon();
	if (Weapon)
		*(float*)(Weapon + 0x24A0) = 0.001f;
}

void C_BaseEntity::SetFOV()
{
	Utils::WritePtr<float>({ (uintptr_t)g_pOffFOV, 0x28, 0x0, 0x3C }, g_Settings::fWepFov, false);
	Utils::WritePtr<float>({ (uintptr_t)g_pOffFOV, 0x28, 0x0, 0x38 }, g_Settings::fCharFov, false);
}

void C_BaseEntity::SetGlow()
{
	auto pBase = *(uintptr_t*)(g_pOffChams + 0xB8);

	// Distance
	*(float*)(pBase + 0x130) = 0.0f;
	*(float*)(pBase + 0x134) = 2.0f;

	// Opacity
	*(float*)(pBase + 0x13C) = 3.0f;

	// rgb
	*(float*)(pBase + 0x110) = 57.f;
	*(float*)(pBase + 0x114) = 255.f;
	*(float*)(pBase + 0x118) = 20.f;
}