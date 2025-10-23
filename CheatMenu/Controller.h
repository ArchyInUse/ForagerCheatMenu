#pragma once

#include <Windows.h>
#include <d3d11.h>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

struct FeatureSettings
{
};

class Controller
{
public:
	static Controller& Get();
	bool IsInited() const { return m_init; }
	void Init(HWND gameWnd, ID3D11Device* device, ID3D11DeviceContext* ctx, IDXGISwapChain* swap);
	void Tick();
	void Render();
	void Shutdown();

private:
	Controller() = default;
	~Controller() = default;

	ID3D11Device* m_device = nullptr;
	ID3D11DeviceContext* m_ctx = nullptr;
	IDXGISwapChain* m_swap = nullptr;
	ID3D11RenderTargetView* m_mainRTV = nullptr;
	HWND m_hwnd = nullptr;
	bool m_visible = true;
	bool m_init = false;
	FeatureSettings* m_settings = nullptr;
};