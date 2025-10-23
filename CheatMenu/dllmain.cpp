#include "pch.h"
#include <windows.h>
#include <string>
#include <vector>
#include <fstream>

// NOTE: This file MUST be compiled to target x86 since the game itself runs as a 32 bit PE, otherwise a mismatch between architecture
// will occur which will lead to the cheat menu not loading properly

using namespace std;

const string baseGame = "Forager.exe";
const uintptr_t moneyBase = 0x0178E08C;
const vector<uintptr_t> moneyOffsets = { 0x0, 0x2C, 0x10, 0x3A8, 0x0 };

const char* detect_graphics_api();

DWORD WINAPI InitThread(LPVOID lp)
{
    MessageBoxA(nullptr, detect_graphics_api(), "DLL Injection Test Caption", MB_ICONINFORMATION);

    // write test (suggested by GPT) of file to test write perms
    {
        std::ofstream f("C:\\temp\\dll_injected.txt", std::ios::app);
        f << "Injected at tick: " << GetTickCount64() << std::endl;
    }

    // another suggestion by GPT to test functionality
    bool toggle = true;
    // need the lib to be unloaded for now for testing graphics API purposes.
    while (false)
    {
        if (GetAsyncKeyState(VK_F1) & 1)
        {
            toggle = !toggle;
            char buf[64];
            sprintf_s(buf, "Toggle now: %s", toggle ? "ON" : "OFF");
            OutputDebugStringA(buf);
        }
        // END ends the loop
        if (GetAsyncKeyState(VK_END) & 1) break;
        Sleep(1000);
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
        break;
    }
    return TRUE;
}

const char* detect_graphics_api()
{
    if (GetModuleHandleA("d3d12.dll")) return "D3D12 (d3d12.dll present)";
    if (GetModuleHandleA("d3d11.dll")) return "D3D11 (d3d11.dll present)";
    if (GetModuleHandleA("d3d9.dll"))  return "D3D9 (d3d9.dll present)";
    if (GetModuleHandleA("dxgi.dll"))  return "DXGI present (likely D3D11/D3D12)";
    if (GetModuleHandleA("vulkan-1.dll")) return "Vulkan (vulkan-1.dll present)";
    if (GetModuleHandleA("opengl32.dll")) return "OpenGL (opengl32.dll present)";
    return "Unknown (no common module found)";
}