#include "DX11Hook.h"
#include "../VR/OpenXRManager.h"
#include <MinHook.h>
#include <iostream>
#include <d3d11.h>

#pragma comment(lib, "d3d11.lib")

// Static initializers
IDXGISwapChain_Present_t DX11Hook::oPresent = nullptr;
IDXGISwapChain_ResizeBuffers_t DX11Hook::oResizeBuffers = nullptr;
DX11Hook* DX11Hook::s_Instance = nullptr;

DX11Hook::DX11Hook() {
    s_Instance = this;
}

DX11Hook::~DX11Hook() {
    Shutdown();
}

bool DX11Hook::Initialize() {
    if (m_Initialized) return true;

    if (MH_Initialize() != MH_OK) {
        std::cerr << "MinHook initialization failed." << std::endl;
        return false;
    }

    if (!CreateDummyDevice()) {
        std::cerr << "Failed to create dummy device for hooking." << std::endl;
        return false;
    }

    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
        std::cerr << "Failed to enable hooks." << std::endl;
        return false;
    }

    m_Initialized = true;
    return true;
}

void DX11Hook::Shutdown() {
    if (m_Initialized) {
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
        m_Initialized = false;
    }
}

// Create a dummy device to get the SwapChain VTable addresses
bool DX11Hook::CreateDummyDevice() {
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1 };

    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 1;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = GetForegroundWindow(); // Just grab any window
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;
    
    // Fallback if no window
    if (!scd.OutputWindow) {
        WNDCLASS wc = { 0 };
        wc.lpfnWndProc = DefWindowProc;
        wc.lpszClassName = "DummyWindow";
        RegisterClass(&wc);
        scd.OutputWindow = CreateWindow("DummyWindow", NULL, 0, 0, 0, 100, 100, NULL, NULL, NULL, NULL);
    }

    ID3D11Device* d3dDevice = nullptr;
    ID3D11DeviceContext* d3dContext = nullptr;
    IDXGISwapChain* swapChain = nullptr;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, featureLevels, 2,
        D3D11_SDK_VERSION, &scd, &swapChain, &d3dDevice, &featureLevel, &d3dContext
    );

    if (FAILED(hr)) {
        std::cerr << "D3D11CreateDeviceAndSwapChain failed: " << std::hex << hr << std::endl;
        return false;
    }

    // Get VTable
    void** vTable = *reinterpret_cast<void***>(swapChain);
    
    // Present is index 8, ResizeBuffers is index 13
    HookFunctions(vTable[8], vTable[13]);

    swapChain->Release();
    d3dDevice->Release();
    d3dContext->Release();
    
    return true;
}

void DX11Hook::HookFunctions(void* pPresent, void* pResizeBuffers) {
    if (MH_CreateHook(pPresent, &GetDetouredPresent, reinterpret_cast<LPVOID*>(&oPresent)) != MH_OK) {
        std::cerr << "Failed to hook Present" << std::endl;
    } else {
        std::cout << "Hooked Present at " << pPresent << std::endl;
    }

    if (MH_CreateHook(pResizeBuffers, &GetDetouredResizeBuffers, reinterpret_cast<LPVOID*>(&oResizeBuffers)) != MH_OK) {
        std::cerr << "Failed to hook ResizeBuffers" << std::endl;
    }
}

// The core hook: this is called every frame
HRESULT __stdcall DX11Hook::GetDetouredPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    if (s_Instance) {
        // First run initialization?
        if (!s_Instance->m_Device) {
            pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&s_Instance->m_Device);
            s_Instance->m_Device->GetImmediateContext(&s_Instance->m_Context);
            std::cout << "First Present: Captured Device Context." << std::endl;
            
            // Initialize VR
            if (s_Instance->m_VrManager.Initialize(s_Instance->m_Device)) {
                std::cout << "VR Manager initialized." << std::endl;
            } else {
                std::cerr << "Failed to initialize VR Manager." << std::endl;
            }
        }

        // Input Handling (Quick & Dirty for MVP)
        static float sep = 0.02f;
        static float conv = 1.0f;
        
        if (GetAsyncKeyState(VK_CONTROL)) {
             if (GetAsyncKeyState(VK_UP) & 1) sep += 0.005f;
             if (GetAsyncKeyState(VK_DOWN) & 1) sep -= 0.005f;
             s_Instance->m_VrManager.SetStereoParams(sep, conv);
        }

        // Head Tracking -> Mouse
        static float lastYaw = 0.0f;
        auto q = s_Instance->m_VrManager.GetHeadPose();
        
        // Simple Quaternion to Yaw (assuming no extreme pitch/roll)
        // Yaw = atan2(2(wy + zx), 1 - 2(y^2 + z^2))
        float siny_cosp = 2 * (q.w * q.y + q.z * q.x);
        float cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z);
        float yaw = std::atan2(siny_cosp, cosy_cosp);
        
        float delta = yaw - lastYaw;
        // Handle wrap around PI
        if (delta > 3.14159f) delta -= 2 * 3.14159f;
        if (delta < -3.14159f) delta += 2 * 3.14159f;
        
        // Threshold to avoid drift when stationary
        if (abs(delta) > 0.001f) {
            // Sensitivity
            int moveX = (int)(delta * 1000.0f); 
            mouse_event(MOUSEEVENTF_MOVE, moveX, 0, 0, 0);
            lastYaw = yaw;
        }

        // Pass BackBuffer to VR system
        ID3D11Texture2D* pBackBuffer = nullptr;
        ID3D11Texture2D* pDepthBuffer = nullptr;
        ID3D11DepthStencilView* pDSV = nullptr;

        // Capture Depth
        s_Instance->m_Context->OMGetRenderTargets(0, nullptr, &pDSV);
        if (pDSV) {
            ID3D11Resource* res = nullptr;
            pDSV->GetResource(&res);
            pDSV->Release(); // OMGetRenderTargets adds ref
            if (res) {
                res->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&pDepthBuffer);
                res->Release(); 
            }
        }

        if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer))) {
            s_Instance->m_VrManager.RenderFrame(pBackBuffer, pDepthBuffer);
            
            pBackBuffer->Release();
        }
        
        if (pDepthBuffer) pDepthBuffer->Release();
    }

    return oPresent(pSwapChain, SyncInterval, Flags);
}

HRESULT __stdcall DX11Hook::GetDetouredResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
    // Release any references before resize
    // TODO: Notify VR Manager to release resources
    
    return oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
}
