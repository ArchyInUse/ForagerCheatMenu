#include "pch.h"
#include "Controller.h"
#include <string>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")


void Controller::Init(HWND gameWnd, ID3D11Device* device, ID3D11DeviceContext* ctx, IDXGISwapChain* swap)
{
	m_settings = new FeatureSettings();
	if (m_init) return;
	m_device = device; if (m_device) m_device->AddRef();
	m_ctx = ctx; if (m_ctx) m_ctx->AddRef();
	m_swap = swap; if (m_swap) m_swap->AddRef();
	m_hwnd = gameWnd;

	// creating a render target
	ID3D11Texture2D* pBackBuffer = nullptr;
	if (SUCCEEDED(m_swap->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer))) {
		m_device->CreateRenderTargetView(pBackBuffer, nullptr, &m_mainRTV);
		pBackBuffer->Release();
	}

	if (!m_mainRTV) {
		OutputDebugStringA("Controller::Init - m_mainRTV IS NULL\n");
	}
	else {
		OutputDebugStringA("Controller::Init - m_mainRTV OK\n");
	}

	// initialize ImGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	OutputDebugStringA("Controller::Init - ImGui_ImplWin32_Init called\n");
	ImGui_ImplWin32_Init(m_hwnd);
	OutputDebugStringA("Controller::Init - ImGui_ImplDX11_Init called\n");
	ImGui_ImplDX11_Init(m_device, m_ctx);
	OutputDebugStringA("Controller::Init - completed \n");
	m_init = true;
	OutputDebugStringA(std::string("Controller::Init - this =" + std::to_string((uintptr_t)this) + "\n").c_str());
}

void Controller::Tick()
{
	// reserved for read operation on neccessary pointers (money for example)
}

static void DrawMenu(FeatureSettings* settings)
{
	ImGui::Begin("Overlay");
	ImGui::Text("Hello from Injected DLL");
	ImGui::End();
}

void Controller::Render()
{ 
	OutputDebugStringA("Controller::Render - entry\n");
	if (!m_init)
	{
		OutputDebugStringA("Controller::Render - NOT inited");
		OutputDebugStringA(std::string("Controller::Render - this =" + std::to_string((uintptr_t)this) + "\n").c_str());
		return;
	}

	if (GetAsyncKeyState(VK_F10) & 1) m_visible = !m_visible;

	if (!m_visible) return;

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	DrawMenu(m_settings);

	ImGui::Render();

	m_ctx->OMSetRenderTargets(1, &m_mainRTV, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void Controller::Shutdown()
{
	OutputDebugStringA("Controller::ShutDown entry");
	if (!m_init) return;

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	if (m_mainRTV) { m_mainRTV->Release(); m_mainRTV = nullptr; }
	if (m_swap) { m_swap->Release(); m_swap = nullptr; }
	if (m_ctx) { m_ctx->Release(); m_ctx = nullptr; }
	if (m_device) { m_device->Release(); m_device = nullptr; }

	m_init = false;
}

Controller& Controller::Get()
{
	static Controller inst;
	return inst;
}