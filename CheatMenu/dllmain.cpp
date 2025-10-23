#include "pch.h"
#include <windows.h>
#include <string>
#include <vector>
#include <fstream>
#include <d3d11.h>
#include <dxgi.h>
#include "MinHook.h"
#include "Controller.h"
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

// NOTE: This file MUST be compiled to target x86 since the game itself runs as a 32 bit PE, otherwise a mismatch between architecture
// will occur which will lead to the cheat menu not loading properly

using namespace std;

const string baseGame = "Forager.exe";
const uintptr_t moneyBase = 0x0178E08C;
const vector<uintptr_t> moneyOffsets = { 0x0, 0x2C, 0x10, 0x3A8, 0x0 };

bool InstallPresentHook();
void RemovePresentHook();

DWORD WINAPI InitThread(LPVOID lp)
{
    Sleep(2000);

    if (!InstallPresentHook())
    {
        MessageBoxA(NULL, "Present hook install failed", "Error", MB_OK);
        return 1;
    }

    while(true)
    {
        Sleep(500);
    }

    HMODULE me = nullptr;
    GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(reinterpret_cast<void*>(&InitThread)),
        &me
    );

    FreeLibraryAndExitThread(me, 0);
    return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        // separating the threads so the process doesnt get stuck loading the DLL
        CreateThread(nullptr, 0, InitThread, nullptr, 0, nullptr);
        break;
    case DLL_PROCESS_DETACH:
        Controller::Get().Shutdown();
        RemovePresentHook();
        break;
    }
    return TRUE;
}

typedef HRESULT(__stdcall* PresentFn)(IDXGISwapChain*, UINT, UINT);
PresentFn oPresent = nullptr;

// our hook
HRESULT __stdcall hkPresent(IDXGISwapChain* pSwap, UINT SyncInterval, UINT Flags) {
    static bool hookedInit = false;
    if (!hookedInit) {
        // try to grab device/context/hwnd from the real swapchain
        ID3D11Device* dev = nullptr;
        if (SUCCEEDED(pSwap->GetDevice(__uuidof(ID3D11Device), (void**)&dev))) {
            ID3D11DeviceContext* ctx = nullptr;
            dev->GetImmediateContext(&ctx);

            DXGI_SWAP_CHAIN_DESC sd;
            pSwap->GetDesc(&sd);
            HWND hwnd = sd.OutputWindow;

            // initialize controller with real device/context/swapchain
            Controller::Get().Init(hwnd, dev, ctx, pSwap);

            if (ctx) ctx->Release();
            if (dev) dev->Release();
        }
        hookedInit = true;
    }

    // allow controller to draw (if initialized)
    Controller::Get().Tick();
    Controller::Get().Render();

    // call original present
    return oPresent(pSwap, SyncInterval, Flags);
}

bool InstallPresentHook() {
    // create a dummy device+swapchain to read vtable pointer
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = 2;
    sd.BufferDesc.Height = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = GetForegroundWindow(); // temporary placeholder
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;

    D3D_FEATURE_LEVEL featureLevel;
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* ctx = nullptr;
    IDXGISwapChain* swap = nullptr;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &sd,
        &swap,
        &device,
        &featureLevel,
        &ctx
    );

    if (FAILED(hr)) {
        // fallback: try WARP or different flags - but for brevity, fail
        return false;
    }

    // read Present pointer from vtable
    void** vtable = *reinterpret_cast<void***>(swap);
    void* presentAddr = vtable[8]; // IDXGISwapChain::Present is usually index 8
    // cleanup dummy objects
    if (swap) swap->Release();
    if (ctx) ctx->Release();
    if (device) device->Release();

    // initialize MinHook and create hook
    if (MH_Initialize() != MH_OK) return false;
    if (MH_CreateHook(presentAddr, &hkPresent, reinterpret_cast<void**>(&oPresent)) != MH_OK) return false;
    if (MH_EnableHook(presentAddr) != MH_OK) return false;
    return true;
}

void RemovePresentHook() {
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
}