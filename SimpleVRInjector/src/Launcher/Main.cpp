#include <windows.h>
#include <iostream>
#include <string>
#include <filesystem>
#include "../Common/Shared.h"

// Simple Injector
bool InjectDLL(HANDLE hProcess, const std::string& dllPath) {
    LPVOID pLoadLibrary = (LPVOID)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
    if (!pLoadLibrary) return false;

    LPVOID pRemoteString = VirtualAllocEx(hProcess, NULL, dllPath.size() + 1, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!pRemoteString) return false;

    if (!WriteProcessMemory(hProcess, pRemoteString, dllPath.c_str(), dllPath.size() + 1, NULL)) {
        VirtualFreeEx(hProcess, pRemoteString, 0, MEM_RELEASE);
        return false;
    }

    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pLoadLibrary, pRemoteString, 0, NULL);
    if (!hThread) {
        VirtualFreeEx(hProcess, pRemoteString, 0, MEM_RELEASE);
        return false;
    }

    WaitForSingleObject(hThread, INFINITE);
    VirtualFreeEx(hProcess, pRemoteString, 0, MEM_RELEASE);
    CloseHandle(hThread);
    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: VrLauncher.exe <path_to_game_exe>" << std::endl;
        return 1;
    }

    std::string gamePath = argv[1];
    std::filesystem::path dllPath = std::filesystem::current_path() / "VrHook.dll";

    if (!std::filesystem::exists(dllPath)) {
        std::cerr << "Error: VrHook.dll not found at " << dllPath << std::endl;
        return 1;
    }

    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };

    std::cout << "Launching: " << gamePath << std::endl;

    if (CreateProcess(gamePath.c_str(), NULL, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi)) {
        std::cout << "Process created. Injecting..." << std::endl;
        
        if (InjectDLL(pi.hProcess, dllPath.string())) {
            std::cout << "Injection successful. Resuming..." << std::endl;
            ResumeThread(pi.hThread);
        } else {
            std::cerr << "Injection failed." << std::endl;
            TerminateProcess(pi.hProcess, 1);
        }

        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        std::cerr << "Failed to start process. Error: " << GetLastError() << std::endl;
    }

    return 0;
}
