#include "Vector.hpp"
#include "Camera.h"

Vector C_Camera::GetViewRight()
{
	return *(Vector*)((uintptr_t)this + 0xD0);
}

Vector C_Camera::GetViewUp()
{
	return *(Vector*)((uintptr_t)this + 0xE0);
}

Vector C_Camera::GetViewForward()
{
	return *(Vector*)((uintptr_t)this + 0xF0);
}

Vector C_Camera::GetViewTranslation()
{
	return *(Vector*)((uintptr_t)this + 0x100);
}

float C_Camera::GetViewFovX()
{
	return *(float*)((uintptr_t)this + 0x110);
}

float C_Camera::GetViewFovY()
{
	return *(float*)((uintptr_t)this + 0x124);
}
