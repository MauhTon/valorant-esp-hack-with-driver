#include "Settings.h"
#include "Menu.h"
#include <iostream>
#include "Globals.h"

void Menu::RenderMenu(nk_context* ctx, ID3D11DeviceContext* d3dctx)
{
	if (nk_begin(ctx, "OverFlow", nk_rect(10, (Globals::g_iWindowHeight / 2) - 180, 180, 300), NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_MOVABLE | NK_WINDOW_TITLE))
	{
		nk_layout_row_dynamic(ctx, 30, 1);
		if (nk_button_label(ctx, "Shutdown"))
			g_Settings::bShutDown = true;
	}

	nk_end(ctx);
	nk_d3d11_render(d3dctx, NK_ANTI_ALIASING_OFF);

	nk_input_begin(ctx);
	nk_input_end(ctx);
} 
