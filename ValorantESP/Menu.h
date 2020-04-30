#pragma once
#include "nuklear_d3d11.h"

class Menu
{
public:
	void RenderMenu(nk_context* ctx, ID3D11DeviceContext* d3dctx);
};