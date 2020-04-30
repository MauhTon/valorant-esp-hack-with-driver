#include "nuklear_d3d11.h"
#include "Hook.h"
#include "Utils.h"
#include "Globals.h"
#include "D3D11Renderer.h"
#include "Settings.h"
#include "SDKMisc.h"
#include "Features.h"
#include "Menu.h"

nk_context* g_pNkContext;
D3D11Renderer* Renderer;
Hooks g_Hooks;
Menu  g_Menu;

IDXGISwapChain* SwapChain = nullptr;

LRESULT CALLBACK DXGIMsgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

uintptr_t* Hooks::CreateDeviceAndSwap()
{
	WNDCLASSEXA wc = { sizeof(WNDCLASSEX), CS_CLASSDC, DXGIMsgProc, 0L, 0L, GetModuleHandleA(NULL), NULL, NULL, NULL, NULL, " ", NULL };
	RegisterClassExA(&wc);

	D3D_FEATURE_LEVEL requestedLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1 };
	D3D_FEATURE_LEVEL obtainedLevel;

	HWND hWnd = CreateWindowA(" ", NULL, WS_OVERLAPPEDWINDOW, 5, 5, 7, 8, NULL, NULL, wc.hInstance, NULL);

	DXGI_SWAP_CHAIN_DESC scd;
	ZeroMemory(&scd, sizeof(scd));

	scd.BufferCount = 1; scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; scd.BufferDesc.RefreshRate.Denominator = 1; scd.OutputWindow = hWnd; scd.BufferDesc.Width = 1;
	scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED; scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	scd.OutputWindow = hWnd; scd.SampleDesc.Count = 1; scd.Windowed = true; scd.BufferDesc.Height = 1; scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED; scd.BufferDesc.RefreshRate.Numerator = 0;

	HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, requestedLevels, sizeof(requestedLevels) / sizeof(D3D_FEATURE_LEVEL), D3D11_SDK_VERSION, &scd, 
		&SwapChain, &g_Hooks.pD3DDevice, &obtainedLevel, &g_Hooks.pD3DContext);

	return (uintptr_t*)SwapChain;
}

void Hooks::HookInit()
{
	Utils::Log("Initializing Hooks!");

	// Get rainbow window handle
	while (!(g_Hooks.hWindow = FindWindowA(NULL, "Rainbow Six")));

	// Get swap chain address and create class object
	g_Hooks.pD3DSwap = std::make_unique<VMTHook>(CreateDeviceAndSwap());

	// Hooks
	g_Hooks.oD3D11Present = reinterpret_cast<D3D11Present_o>(g_Hooks.pD3DSwap->Hook(Hooks::HookedPresent, 8));
	g_Hooks.pOriginalWNDProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(g_Hooks.hWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_Hooks.WndProc)));

	Utils::Log("Finished!");
	return;
}

D3D11_VIEWPORT vpNew, vpOld; UINT nViewPorts = 1;
HRESULT __stdcall Hooks::HookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	static bool Init = true;
	if (Init)
	{
		pSwapChain->GetDevice(__uuidof(g_Hooks.pD3DDevice), reinterpret_cast<void**>(&g_Hooks.pD3DDevice));
		g_Hooks.pD3DDevice->GetImmediateContext(&g_Hooks.pD3DContext);

		ID3D11Texture2D* renderTargetTexture = nullptr;
		if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<PVOID*>(&renderTargetTexture))))
		{
			g_Hooks.pD3DDevice->CreateRenderTargetView(renderTargetTexture, NULL, &g_Hooks.pD3DRenderTargetView);
			renderTargetTexture->Release();
		}

		Renderer = new D3D11Renderer(pSwapChain);
		Renderer->Initialize();

		vpNew.Width = 1920;
		vpNew.Height = 1080;

		g_Hooks.pD3DContext->RSGetViewports(&nViewPorts, &vpOld);

		g_pNkContext = nk_d3d11_init(g_Hooks.pD3DDevice, Globals::g_iWindowWidth, Globals::g_iWindowHeight, MAX_VERTEX_BUFFER, MAX_INDEX_BUFFER);
		
		nk_font_atlas* pNkAtlas;
		nk_d3d11_font_stash_begin(&pNkAtlas);
		nk_d3d11_font_stash_end();
		set_style(g_pNkContext, THEME_DARK);
		
		Init = false;
	}
	
	// Set new viewport 
	g_Hooks.pD3DContext->RSSetViewports(1, &vpNew);

	g_Hooks.pD3DContext->OMSetRenderTargets(1, &g_Hooks.pD3DRenderTargetView, NULL);
	Renderer->BeginScene();

	if (g_pEngine->IsInGame())
	{
		g_pLocalEntity = g_pEngine->GetLocal();

		Features::RenderESP(Renderer, g_pNkContext);

		if (Globals::PressedKeys[VK_RBUTTON])
			Features::DoAimbot();

		g_pLocalEntity->NoRecoil();
		g_pLocalEntity->NoSpread();
		g_pLocalEntity->NoReload();
		g_pLocalEntity->SetGlow();
		g_pLocalEntity->SetSpeed();
		g_pLocalEntity->SetFOV();
	}

	if (g_Settings::bMenu)
		g_Menu.RenderMenu(g_pNkContext, g_Hooks.pD3DContext);

	Renderer->EndScene();

	// restore old viewport 
	g_Hooks.pD3DContext->RSSetViewports(1, &vpOld);

	if (g_Settings::bShutDown)
	{
		// call original first so dont fuck up game
		g_Hooks.oD3D11Present(pSwapChain, SyncInterval, Flags);

		// unhook
		g_Hooks.pD3DSwap->UnHook(8);
		reinterpret_cast<WNDPROC>(SetWindowLongPtr(g_Hooks.hWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_Hooks.pOriginalWNDProc)));

		// shut down menu
		nk_d3d11_shutdown();
		g_Hooks.pD3DContext->Release();
		g_Hooks.pD3DContext->Release();
		pSwapChain		   ->Release();

		FreeConsole();
		//FreeLibraryAndExitThread(static_cast<HMODULE>(g_Settings::hModule), 1);
	}

	return g_Hooks.oD3D11Present(pSwapChain, SyncInterval, Flags);
}

LRESULT Hooks::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// for now as a lambda, to be transfered somewhere
	// Thanks uc/WasserEsser for pointing out my mistake!
	// Working when you HOLD th button, not when you press it.
	const auto getButtonHeld = [uMsg, wParam](bool& bButton, int vKey)
	{
		if (wParam != 2)
			return;

		if (uMsg == 256)
			bButton = true;
		else if (uMsg == 257)
			bButton = false;
	};

	const auto getButtonToggle = [uMsg, wParam](bool& bButton, int vKey)
	{
		if (wParam != vKey) // 45 
			return;

		if (uMsg == WM_KEYUP) // KEY_UP == 257
			bButton = !bButton;
	};

	getButtonToggle(g_Settings::bMenu, VK_INSERT);

	switch (uMsg)
	{
	case WM_SIZE:	break;
	case WM_RBUTTONDOWN:
		Globals::PressedKeys[VK_RBUTTON] = true;
		break;
	case WM_RBUTTONUP:
		Globals::PressedKeys[VK_RBUTTON] = false;
		break;
	default:
		break;
	}

	// our wndproc capture fn
	if (g_Settings::bMenu)
	{
		if (nk_d3d11_handle_event(hWnd, uMsg, wParam, lParam));
			return true;
	}
	
	// Call original wndproc to make game use input again
	return CallWindowProcA(g_Hooks.pOriginalWNDProc, hWnd, uMsg, wParam, lParam);
}