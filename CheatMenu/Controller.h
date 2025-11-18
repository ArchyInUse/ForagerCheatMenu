#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <vector>
#include <iomanip>
#include <sstream>
#include <map>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "PatchHelpers.h"

struct FeatureSettings
{
	bool lockStamina = false;
	bool lockHealth = false;
	bool multiplySell = false;
	bool infiniteDamage = false;
	double previousDamage = -1;
	Patching::Patch* multiplySellPatch = nullptr;
};

struct GameData {
	double* money = nullptr;
	double* xp = nullptr;
	double* stamina = nullptr;
	double* health = nullptr;
	double* damage = nullptr;
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
	bool MenuVisible() const { return m_visible; }
	void RefreshGameData();
	void DrawMenu(FeatureSettings* settings, GameData* data);

private:
	Controller() = default;
	~Controller() = default;

	void* GetModuleBaseAddress(const char* moduleName);
	uintptr_t GetModuleEntryRVA(const char* moduleName);
	uintptr_t GetModuleEntryVA(const char* moduleName);
	template<typename T>
	T* ResolvePtr(uintptr_t base, const std::vector<uintptr_t>& offsets)
	{
		uintptr_t addr = base;
		for (int i = 0; i < offsets.size(); ++i)
		{
			auto last = addr;
			addr += offsets[i];
			if (i < offsets.size() - 1)
			{
				addr = *reinterpret_cast<uintptr_t*>(addr);
				char buf[256];
				_snprintf_s(buf, _TRUNCATE, "Controller::ResolvePtr - [%d] [0x%08X + 0x%X] => 0x%08X\n",
					i, (unsigned)last, (unsigned)offsets[i], (unsigned)addr);
				OutputDebugStringA(std::string(buf).c_str());
				if (!addr) return nullptr;
			}
		}
		return reinterpret_cast<T*>(addr);
	}
	void PatchMemory(uintptr_t addr, BYTE* bytes, size_t size);
	void PatchNOP(uintptr_t addr, size_t size);

	ID3D11Device* m_device = nullptr;
	ID3D11DeviceContext* m_ctx = nullptr;
	IDXGISwapChain* m_swap = nullptr;
	ID3D11RenderTargetView* m_mainRTV = nullptr;
	HWND m_hwnd = nullptr;
	bool m_visible = true;
	bool m_init = false;
	FeatureSettings* m_settings = nullptr;
	GameData* m_data = nullptr;
	const std::map<int, int> xpPerLevel = {
		{1, 80}, {2, 92}, {3, 106}, {4, 140}, {5, 141},
		{6, 163}, {7, 188}, {8, 217}, {9, 250}, {10, 288},
		{11, 332}, {12, 382}, {13, 440}, {14, 506}, {15, 582},
		{16, 670}, {17, 771}, {18, 887}, {19, 1021}, {20, 1175},
		{21, 1352}, {22, 1555}, {23, 1789}, {24, 2058}, {25, 2367},
		{26, 2723}, {27, 3132}, {28, 3602}, {29, 4143}, {30, 4765},
		{31, 5480}, {32, 6302}, {33, 7248}, {34, 8336}, {35, 9587},
		{36, 11026}, {37, 12680}, {38, 14582}, {39, 16770}, {40, 19286},
		{41, 22179}, {42, 25506}, {43, 29332}, {44, 33732}, {45, 38792},
		{46, 44611}, {47, 51303}, {48, 58999}, {49, 67849}, {50, 78027},
		{51, 89732}, {52, 103192}, {53, 118671}, {54, 136472}, {55, 156943},
		{56, 180485}, {57, 207558}, {58, 238692}, {59, 274496}, {60, 315671},
		{61, 363022}, {62, 417476}, {63, 480098}, {64, 552113}, {65, 634930}
	};

};