#pragma once
#include <d3d11.h>
#include <iostream>

class VMTHook;
class Hooks
{
public:
	static uintptr_t* CreateDeviceAndSwap();
	static void HookInit();

	// Hooked functions
	static HRESULT __stdcall  HookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT flags);
	static LRESULT __stdcall  WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// Original functions
	typedef HRESULT(__stdcall* D3D11Present_o)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);

private:
	D3D11Present_o			 oD3D11Present			 = nullptr;
	WNDPROC				     pOriginalWNDProc		 = nullptr;

	HWND hWindow									 = nullptr;
	std::unique_ptr<VMTHook> pD3DSwap				 = nullptr;

	ID3D11Device*			 pD3DDevice				 = nullptr;
	ID3D11DeviceContext*	 pD3DContext			 = nullptr;
	ID3D11Texture2D*		 pD3DRenderTargetTexture = nullptr;
	ID3D11RenderTargetView*  pD3DRenderTargetView    = nullptr;
};

class VMTHook
{
public:
	VMTHook(void* Instance) : ppBaseTable(reinterpret_cast<uintptr_t**>(Instance)) { this->pOriginalVMT = *ppBaseTable; }

	uintptr_t Hook(void* NewFunc, const std::size_t Index)
	{
		// save orignal function address
		this->pOrgVFunc = (uintptr_t*)this->pOriginalVMT[Index];

		// change vmt function pointer
		this->pOriginalVMT[Index] = reinterpret_cast<uintptr_t>(NewFunc);
		return (uintptr_t)this->pOrgVFunc;
	}

	void UnHook(const std::size_t Index) { this->pOriginalVMT[Index] = (uintptr_t)this->pOrgVFunc; };

	template <class T>
	T GetOriginal() 
	{ 
		return reinterpret_cast<T>(this->mOrgVFunc); 
	};

private:
	uintptr_t*   pOriginalVMT = nullptr; // Store original vtable
	uintptr_t*   pOrgVFunc = nullptr; // Store original vtable
	uintptr_t**  ppBaseTable = nullptr;
};



