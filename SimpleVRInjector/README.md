# SimpleVRInjector

A simplified "VorpX-like" tool to inject a VR runtime into DirectX 11 games.

## Logic
1.  **Launcher** creates the game process suspended.
2.  **Launcher** injects `VrHook.dll`.
3.  **VrHook.dll** hooks `D3D11CreateDeviceAndSwapChain` and `IDXGISwapChain::Present`.
4.  On the first frame, it initializes OpenXR using the game's D3D11 Device.
5.  Every frame, it copies the BackBuffer to the OpenXR Swapchain and displays it in the headset.

## Prerequisites (Windows)
-   Visual Studio 2019/2022 (C++ Desktop Development).
-   CMake 3.20+.
-   SteamVR or Oculus App (set as OpenXR Runtime).

## Building
```powershell
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## Running
1.  Ensure SteamVR/Oculus is running.
2.  Run the launcher:
    ```powershell
    .\bin\Release\VrLauncher.exe "C:\Path\To\Game.exe"
    ```
3.  Put on headset. The game should appear floating in front of you.

## Troubleshooting
-   **Crash on startup**: Anti-cheat may block injection. Try a simple game or demo (e.g., Unity demo without IL2CPP).
-   **No VR**: Check the console window launched by the DLL for "[VrHook]" logs. If "Failed to initialize VR", check OpenXR Runtime status.
