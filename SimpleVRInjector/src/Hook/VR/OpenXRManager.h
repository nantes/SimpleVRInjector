#pragma once

#include <d3d11.h>
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <vector>

class OpenXRManager {
public:
    OpenXRManager();
    ~OpenXRManager();

    bool Initialize(ID3D11Device* pDevice);
    void Shutdown();
    
    // Called every frame from Present()
    void RenderFrame(ID3D11Texture2D* gameTexture, ID3D11Texture2D* depthTexture = nullptr);

private:
    bool CreateInstance();
    bool GetSystem();
    bool CreateSession(ID3D11Device* pDevice);
    bool CreateSwapchain();
    
    XrInstance m_Instance = XR_NULL_HANDLE;
    XrSystemId m_SystemId = XR_NULL_SYSTEM_ID;
    XrSession m_Session = XR_NULL_HANDLE;
    XrSpace m_AppSpace = XR_NULL_HANDLE;
    
    // Swapchain
    XrSwapchain m_Swapchain = XR_NULL_HANDLE;
    std::vector<XrSwapchainImageD3D11KHR> m_SwapchainImages;
    uint32_t m_ViewCount = 0;
    std::vector<XrViewConfigurationView> m_ViewConfigs;
    
    // State
    bool m_SessionRunning = false;
    XrSessionState m_SessionState = XR_SESSION_STATE_UNKNOWN;
    
    ID3D11Device* m_D3DDevice = nullptr;
    ID3D11DeviceContext* m_D3DContext = nullptr;

    // Stereo Resources
    bool m_StereoInitialized = false;
    ID3D11VertexShader* m_VS = nullptr;
    ID3D11PixelShader* m_PS = nullptr;
    ID3D11SamplerState* m_Sampler = nullptr;
    ID3D11Buffer* m_ParamBuffer = nullptr;
    
    struct CBParams {
        float Separation;
        float Convergence;
        float EyeSign;
        float Padding;
    };
    
    CBParams m_Params = { 0.02f, 1.0f, 1.0f, 0.0f };
    void SetStereoParams(float sep, float conv) { m_Params.Separation = sep; m_Params.Convergence = conv; }

    struct Quat { float x, y, z, w; };
    Quat GetHeadPose() { return m_HeadPose; }

    bool InitStereoResources();
    void RenderStereo(ID3D11Texture2D* color, ID3D11Texture2D* depth, ID3D11RenderTargetView* rtv, int eye);
    
    Quat m_HeadPose = { 0, 0, 0, 1 };
};
