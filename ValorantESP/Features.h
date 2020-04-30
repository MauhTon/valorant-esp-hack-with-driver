#pragma once
#include "D3D11Renderer.h"

namespace Features
{
	void RenderESP(D3D11Renderer* Render, nk_context* g_pNkContext);
	bool DoAimbot();
}