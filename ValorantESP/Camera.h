#pragma once

class C_Camera
{
public:
	Vector GetViewRight();
	Vector GetViewUp();
	Vector GetViewForward();
	Vector GetViewTranslation();
	float GetViewFovX();
	float GetViewFovY();
};