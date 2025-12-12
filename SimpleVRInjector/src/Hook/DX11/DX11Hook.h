#pragma once

#include <d3d11.h>
#include <dxgi.h>
#include <vector>
#include <functional>

// Function pointer types for original functions
typedef HRESULT(__stdcall* IDXGISwapChain_Present_t)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef HRESULT(__stdcall* IDXGISwapChain_ResizeBuffers_t)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

class DX11Hook {
public:
    DX11Hook();
    ~DX11Hook();

    bool Initialize();
    void Shutdown();

    // Hook callbacks
    static HRESULT __stdcall GetDetouredPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
    static HRESULT __stdcall GetDetouredResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

private:
    bool CreateDummyDevice();
    void HookFunctions(void* pPresent, void* pResizeBuffers);

    static IDXGISwapChain_Present_t oPresent;
    static IDXGISwapChain_ResizeBuffers_t oResizeBuffers;

    static DX11Hook* s_Instance;
    
    // Internal State
    bool m_Initialized = false;
    ID3D11Device* m_Device = nullptr;
    ID3D11DeviceContext* m_Context = nullptr;
    
    // VR Manager
    OpenXRManager m_VrManager;
};
