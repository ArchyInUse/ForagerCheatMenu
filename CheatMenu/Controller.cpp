#include "pch.h"
#include "Controller.h"
#include <string>
#include <vector>
#include <array>
#include <TlHelp32.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

static WNDPROC g_OldWndProc = nullptr;
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

// offsets and constant pointers
namespace offsets
{
	const char* executableBaseName = "Forager.exe";
	const uintptr_t playerOffset = 0x0178E08C;

	// const std::vector<uintptr_t> moneyOffsets = { 0x0, 0x2C, 0x10, 0x3A8, 0x0 };
	const std::vector<uintptr_t> moneyOffsets = { 0x0, 0x2C, 0x10, 0x3A8, 0x0 };
	const std::vector<uintptr_t> xpOffsets = { 0x0, 0x2c, 0x10, 0x834, 0x0 };
	const std::vector<uintptr_t> staminaOffsets = { 0x0, 0x2c, 0x10, 0x90, 0x20 };
	const std::vector<uintptr_t> healthOffsets = { 0x4C, 0x2C, 0x10, 0x990, 0x1A0 };
}

namespace patches {
	// base executable + healthInstruction
	const uintptr_t healthInstruction = 0x12B8046;
	const std::array<BYTE, 4> healthOriginalInstructions = {0xF2, 0x0F, 0x11, 0x07};
}

uintptr_t executableBasePointer = 0;

LRESULT CALLBACK HookedWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return 0;

	// block game from getting mouse inputs when menu is visible
	if (Controller::Get().MenuVisible()) {
		ImGuiIO& io = ImGui::GetIO();
		const bool mouse_msg =
			uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP ||
			uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONUP ||
			uMsg == WM_MBUTTONDOWN || uMsg == WM_MBUTTONUP ||
			uMsg == WM_MOUSEMOVE || uMsg == WM_MOUSEWHEEL || uMsg == WM_MOUSEHWHEEL;
		const bool key_msg = (uMsg == WM_KEYDOWN || uMsg == WM_KEYUP || uMsg == WM_CHAR);

		if ((mouse_msg && io.WantCaptureMouse) || (key_msg && io.WantCaptureKeyboard))
			return 0; 
	}
	return CallWindowProc(g_OldWndProc, hWnd, uMsg, wParam, lParam);
}

void Controller::Init(HWND gameWnd, ID3D11Device* device, ID3D11DeviceContext* ctx, IDXGISwapChain* swap)
{
	if (m_init) return;

	g_OldWndProc = (WNDPROC)SetWindowLongW(gameWnd, GWLP_WNDPROC, (LONG_PTR)HookedWndProc);

	m_settings = new FeatureSettings();
	m_device = device; if (m_device) m_device->AddRef();
	m_ctx = ctx; if (m_ctx) m_ctx->AddRef();
	m_swap = swap; if (m_swap) m_swap->AddRef();
	m_hwnd = gameWnd;

	ID3D11Texture2D* pBackBuffer = nullptr;
	if (SUCCEEDED(m_swap->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer))) {
		m_device->CreateRenderTargetView(pBackBuffer, nullptr, &m_mainRTV);
		pBackBuffer->Release();
	}

	if (!m_mainRTV) {
		OutputDebugStringA("Controller::Init - m_mainRTV is NULL\n");
	}
	else {
		OutputDebugStringA("Controller::Init - m_mainRTV OK\n");
	}

	// initialize ImGUI
	IMGUI_CHECKVERSION();
	
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	// OutputDebugStringA("Controller::Init - ImGui_ImplWin32_Init called\n");
	ImGui_ImplWin32_Init(m_hwnd);
	// OutputDebugStringA("Controller::Init - ImGui_ImplDX11_Init called\n");
	ImGui_ImplDX11_Init(m_device, m_ctx);
	// OutputDebugStringA("Controller::Init - completed \n");
	m_init = true;
	// OutputDebugStringA(std::string("Controller::Init - this =" + std::to_string((uintptr_t)this) + "\n").c_str());
	m_data = new GameData({});
	RefreshGameData();
}

void Controller::Tick()
{
	return;
}

void Controller::DrawMenu(FeatureSettings* settings, GameData* data)
{
	ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
	ImGui::Begin("Overlay");
	ImGui::Text("FCM - Forager Cheat Menu");

	// mostly for debug right now
	ImGui::Text("Money: %lf", *data->money);
	ImGui::Text("Stamina: %lf", *data->stamina);

	// stamina controls (infinite in this case means locking at 72.0 as that is the maximum from what I can gather
	ImGui::Text("Infinite Stamina?");
	ImGui::SameLine();
	ImGui::Checkbox("##lockStamina", &m_settings->lockStamina);

	if(m_settings->lockStamina)
	{
		*this->m_data->stamina = 72;
	}

	ImGui::Text("Infinite Health?");
	ImGui::SameLine();
	ImGui::Checkbox("##lockHealth", &m_settings->lockHealth);

	if (m_settings->lockHealth)
	{
		*this->m_data->health = 30;
	}

	// Button Controls
	if (ImGui::Button("Double Gold"))
	{
		*this->m_data->money *= 2;
	}
	if (ImGui::Button("Double XP"))
	{
		*this->m_data->xp *= 2;
	}

	double moneyCopy = *this->m_data->money;
	double xpCopy = *this->m_data->xp;

	// Setters 
	ImGui::Text("Set Money: ");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(150.0f);
	if (ImGui::InputDouble("##money", &moneyCopy))
	{
		*this->m_data->money = moneyCopy;
	}

	ImGui::Text("Set XP: ");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(150.0f);
	if (ImGui::InputDouble("##xp", &xpCopy))
	{
		*this->m_data->xp = xpCopy;
	}

	ImGui::End();
}

void Controller::RefreshGameData()
{
	// NOTE: move base pointer evaluation to init
	if (executableBasePointer == NULL)
	{
		auto moduleBase = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
		executableBasePointer = moduleBase + offsets::playerOffset;
	}

	// note: need to dereference the base executable + player offset first, then send it to resolveptr, as the offsets happen after an initial
	// deref, not just by the addr
	auto executableBasePtrDereffed = *reinterpret_cast<uintptr_t*>(executableBasePointer);

	this->m_data->money = ResolvePtr<double>(
		executableBasePtrDereffed,
		offsets::moneyOffsets);

	this->m_data->xp = ResolvePtr<double>(
		executableBasePtrDereffed,
		offsets::xpOffsets);

	this->m_data->stamina = ResolvePtr<double>(
		executableBasePtrDereffed,
		offsets::staminaOffsets);

	// found static health offset to be +512 from XP, it's unneccessary 
	this->m_data->health = this->m_data->xp + 512;
}

void Controller::PatchMemory(uintptr_t addr, BYTE* bytes, size_t size)
{

}

void Controller::PatchNOP(uintptr_t addr, size_t size)
{
	DWORD old;
	if (!VirtualProtect(reinterpret_cast<LPVOID>(addr), size, PAGE_EXECUTE_READWRITE, &old))
	{
		OutputDebugStringA("Controller::PatchNOP - Failed to unprotect memory, returning");
		return;
	}
	memset(reinterpret_cast<void*>(addr), 0x90, size);
	DWORD tmp;
	VirtualProtect(reinterpret_cast<LPVOID>(addr), size, old, &tmp);
	FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<LPCVOID>(addr), size);
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

	DrawMenu(m_settings, m_data);

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

void* Controller::GetModuleBaseAddress(const char* moduleName)
{
	HMODULE mod = GetModuleHandleA(moduleName);
	if (!mod) return 0;
	auto t = (int)mod;
	auto r = static_cast<void*>(mod);
	return (void*)t;
}

uintptr_t Controller::GetModuleEntryRVA(const char* moduleName)
{
	HMODULE mod = GetModuleHandleA(moduleName);
	if (!mod) return 0;

	auto dos = reinterpret_cast<PIMAGE_DOS_HEADER>(mod);
	auto nt = reinterpret_cast<PIMAGE_NT_HEADERS>(
		reinterpret_cast<uint8_t*>(mod) + dos->e_lfanew);

	return nt->OptionalHeader.AddressOfEntryPoint; // same RVA CE shows
}

uintptr_t Controller::GetModuleEntryVA(const char* moduleName)
{
	HMODULE mod = GetModuleHandleA(moduleName);
	if (!mod) return 0;
	return reinterpret_cast<uintptr_t>(mod) + GetModuleEntryRVA(moduleName);
}


