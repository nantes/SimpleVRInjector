#include <windows.h>
#include <iostream>
#include "DX11/DX11Hook.h"

// Singleton instance of our Hook Manager
DX11Hook g_Hook;

DWORD WINAPI MainThread(LPVOID lpParam) {
    // Open console for debugging
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONERR$", "w", stderr);
    
    std::cout << "[VrHook] Loaded into process." << std::endl;

    if (g_Hook.Initialize()) {
        std::cout << "[VrHook] Hooks initialized." << std::endl;
    } else {
        std::cerr << "[VrHook] Failed to initialize hooks." << std::endl;
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        CreateThread(NULL, 0, MainThread, NULL, 0, NULL);
        break;
    case DLL_PROCESS_DETACH:
        g_Hook.Shutdown();
        break;
    }
    return TRUE;
}
