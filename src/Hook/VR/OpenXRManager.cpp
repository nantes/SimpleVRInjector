#include "OpenXRManager.h"
#include <iostream>
#include <string>
#include <vector>

// Helper macros for error checking
#define CHECK_XR(x, msg) if (XR_FAILED(x)) { std::cerr << "[OpenXR] Error: " << msg << std::endl; return false; }

OpenXRManager::OpenXRManager() {}

OpenXRManager::~OpenXRManager() {
    Shutdown();
}

bool OpenXRManager::Initialize(ID3D11Device* pDevice) {
    if (!pDevice) return false;
    m_D3DDevice = pDevice;
    m_D3DDevice->GetImmediateContext(&m_D3DContext);

    if (!CreateInstance()) return false;
    if (!GetSystem()) return false;
    if (!CreateSession(pDevice)) return false;
    if (!CreateSwapchain()) return false;

    // Create Reference Space (Local or Stage)
    XrReferenceSpaceCreateInfo spaceInfo{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
    spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    spaceInfo.poseInReferenceSpace.orientation.w = 1.0f;
    if (XR_FAILED(xrCreateReferenceSpace(m_Session, &spaceInfo, &m_AppSpace))) {
        return false;
    }

    std::cout << "[OpenXR] Initialized successfully." << std::endl;
    return true;
}

void OpenXRManager::Shutdown() {
    if (m_Swapchain != XR_NULL_HANDLE) xrDestroySwapchain(m_Swapchain);
    if (m_Session != XR_NULL_HANDLE) xrDestroySession(m_Session);
    if (m_Instance != XR_NULL_HANDLE) xrDestroyInstance(m_Instance);
}

bool OpenXRManager::CreateInstance() {
    // Enable D3D11 extension
    const char* extensions[] = { "XR_KHR_D3D11_enable" };
    
    XrInstanceCreateInfo createInfo{ XR_TYPE_INSTANCE_CREATE_INFO };
    strcpy_s(createInfo.applicationInfo.applicationName, "SimpleVRInjector");
    createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
    createInfo.enabledExtensionCount = 1;
    createInfo.enabledExtensionNames = extensions;

    CHECK_XR(xrCreateInstance(&createInfo, &m_Instance), "Failed to create instance");
    return true;
}

bool OpenXRManager::GetSystem() {
    XrSystemGetInfo systemInfo{ XR_TYPE_SYSTEM_GET_INFO };
    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    CHECK_XR(xrGetSystem(m_Instance, &systemInfo, &m_SystemId), "Failed to get system");
    return true;
}

bool OpenXRManager::CreateSession(ID3D11Device* pDevice) {
    XrGraphicsBindingD3D11KHR binding{ XR_TYPE_GRAPHICS_BINDING_D3D11_KHR };
    binding.device = pDevice;

    XrSessionCreateInfo sessionInfo{ XR_TYPE_SESSION_CREATE_INFO };
    sessionInfo.next = &binding;
    sessionInfo.systemId = m_SystemId;
    CHECK_XR(xrCreateSession(m_Instance, &sessionInfo, &m_Session), "Failed to create session");
    return true;
}

bool OpenXRManager::CreateSwapchain() {
    // Get view configs
    uint32_t viewCount = 0;
    xrEnumerateViewConfigurationViews(m_Instance, m_SystemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &viewCount, nullptr);
    m_ViewConfigs.resize(viewCount, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
    xrEnumerateViewConfigurationViews(m_Instance, m_SystemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, viewCount, &viewCount, m_ViewConfigs.data());
    m_ViewCount = viewCount;

    // Create swapchain using left eye resolution (assume symmetric)
    XrSwapchainCreateInfo swapchainInfo{ XR_TYPE_SWAPCHAIN_CREATE_INFO };
    swapchainInfo.arraySize = 2; // Stereo
    swapchainInfo.format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // Standard
    swapchainInfo.width = m_ViewConfigs[0].recommendedImageRectWidth;
    swapchainInfo.height = m_ViewConfigs[0].recommendedImageRectHeight;
    swapchainInfo.sampleCount = 1;
    swapchainInfo.faceCount = 1;
    swapchainInfo.mipCount = 1;

    CHECK_XR(xrCreateSwapchain(m_Session, &swapchainInfo, &m_Swapchain), "Failed to create swapchain");

    // Get images
    uint32_t imageCount = 0;
    xrEnumerateSwapchainImages(m_Swapchain, 0, &imageCount, nullptr);
    m_SwapchainImages.resize(imageCount, { XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR });
    xrEnumerateSwapchainImages(m_Swapchain, imageCount, &imageCount, (XrSwapchainImageBaseHeader*)m_SwapchainImages.data());

    return true;
}

void OpenXRManager::RenderFrame(ID3D11Texture2D* gameTexture, ID3D11Texture2D* depthTexture) {
    // 1. Poll Events
    XrEventDataBuffer eventData{ XR_TYPE_EVENT_DATA_BUFFER };
    while (xrPollEvent(m_Instance, &eventData) == XR_SUCCESS) {
        if (eventData.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
            auto* stateEvent = (XrEventDataSessionStateChanged*)&eventData;
            m_SessionState = stateEvent->state;
            if (m_SessionState == XR_SESSION_STATE_READY) {
                XrSessionBeginInfo beginInfo{ XR_TYPE_SESSION_BEGIN_INFO };
                beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
                xrBeginSession(m_Session, &beginInfo);
                m_SessionRunning = true;
            } else if (m_SessionState == XR_SESSION_STATE_STOPPING) {
                xrEndSession(m_Session);
                m_SessionRunning = false;
            }
        }
    }

    if (!m_SessionRunning) return;

    // 2. Wait Frame
    XrFrameState frameState{ XR_TYPE_FRAME_STATE };
    xrWaitFrame(m_Session, nullptr, &frameState);

    // 3. Begin Frame
    xrBeginFrame(m_Session, nullptr);

    // 4. Render Layers
    std::vector<XrCompositionLayerBaseHeader*> layers;
    XrCompositionLayerProjection layer{ XR_TYPE_COMPOSITION_LAYER_PROJECTION };
    std::vector<XrCompositionLayerProjectionView> projectionViews(m_ViewCount);
    
    if (frameState.shouldRender) {
        // Acquire Swapchain Image
        uint32_t imageIndex;
        XrSwapchainImageAcquireInfo acquireInfo{ XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
        xrAcquireSwapchainImage(m_Swapchain, &acquireInfo, &imageIndex);

        XrSwapchainImageWaitInfo waitInfo{ XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
        waitInfo.timeout = XR_INFINITE_DURATION;
        xrWaitSwapchainImage(m_Swapchain, &waitInfo);

        // Blit Game Texture to Swapchain Image (Left and Right)
        ID3D11Texture2D* targetTex = m_SwapchainImages[imageIndex].texture;

        if (depthTexture && m_StereoInitialized) {
             // Create RTV for Left Eye
             D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
             rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
             rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
             rtvDesc.Texture2DArray.FirstArraySlice = 0;
             rtvDesc.Texture2DArray.ArraySize = 1;
             
             ID3D11RenderTargetView* rtvLeft = nullptr;
             m_D3DDevice->CreateRenderTargetView(targetTex, &rtvDesc, &rtvLeft);
             RenderStereo(gameTexture, depthTexture, rtvLeft, 0);
             if (rtvLeft) rtvLeft->Release();

             // Right Eye
             rtvDesc.Texture2DArray.FirstArraySlice = 1;
             ID3D11RenderTargetView* rtvRight = nullptr;
             m_D3DDevice->CreateRenderTargetView(targetTex, &rtvDesc, &rtvRight);
             RenderStereo(gameTexture, depthTexture, rtvRight, 1);
             if (rtvRight) rtvRight->Release();
        } else {
            // Simple Copy (Fallback)
            m_D3DContext->CopySubresourceRegion(targetTex, 0, 0, 0, 0, gameTexture, 0, NULL);
            m_D3DContext->CopySubresourceRegion(targetTex, 1, 0, 0, 0, gameTexture, 0, NULL);
        }

        xrReleaseSwapchainImage(m_Swapchain, nullptr);

        // Setup Layer
        layer.space = m_AppSpace;
        
        // Get Views to fill FOV
        XrViewLocateInfo viewLocateInfo{ XR_TYPE_VIEW_LOCATE_INFO };
        viewLocateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        viewLocateInfo.displayTime = frameState.predictedDisplayTime;
        viewLocateInfo.space = m_AppSpace;
        
        XrViewState viewState{ XR_TYPE_VIEW_STATE };
        std::vector<XrView> views(m_ViewCount, { XR_TYPE_VIEW });
        xrLocateViews(m_Session, &viewLocateInfo, &viewState, m_ViewCount, &m_ViewCount, views.data());

        for (uint32_t i = 0; i < m_ViewCount; i++) {
            projectionViews[i].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
            projectionViews[i].pose = views[i].pose;
            projectionViews[i].fov = views[i].fov;
            projectionViews[i].subImage.swapchain = m_Swapchain;
            projectionViews[i].subImage.imageRect.offset = { 0, 0 };
            projectionViews[i].subImage.imageRect.extent = { (int32_t)m_ViewConfigs[0].recommendedImageRectWidth, (int32_t)m_ViewConfigs[0].recommendedImageRectHeight };
            projectionViews[i].subImage.imageArrayIndex = i; // Left=0, Right=1
            
            // Store Left Eye Orientation as Head Pose (Approx)
            if (i == 0) {
                 m_HeadPose = { views[i].pose.orientation.x, views[i].pose.orientation.y, views[i].pose.orientation.z, views[i].pose.orientation.w };
            }
        }
        
        layer.viewCount = (uint32_t)projectionViews.size();
        layer.views = projectionViews.data();
        layers.push_back((XrCompositionLayerBaseHeader*)&layer);
    }

    // 5. End Frame
    XrFrameEndInfo endInfo{ XR_TYPE_FRAME_END_INFO };
    endInfo.displayTime = frameState.predictedDisplayTime;
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    endInfo.layerCount = (uint32_t)layers.size();
    endInfo.layers = layers.data();
    xrEndFrame(m_Session, &endInfo);
}

// ==============================================================================================
// Stereo Logic
// ==============================================================================================
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

bool OpenXRManager::InitStereoResources() {
    if (m_StereoInitialized) return true;

    // 1. Compile VS (Full Screen Quad)
    const char* vsCode = R"(
        struct VS_INPUT { uint VertexID : SV_VertexID; };
        struct PS_INPUT { float4 Pos : SV_POSITION; float2 Tex : TEXCOORD0; };
        PS_INPUT VS(VS_INPUT input) {
            PS_INPUT output;
            float2 grid = float2((input.VertexID << 1) & 2, input.VertexID & 2);
            output.Pos = float4(grid * float2(2, -2) + float2(-1, 1), 0, 1);
            output.Tex = grid;
            return output;
        }
    )";
    
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* errBlob = nullptr;
    if (FAILED(D3DCompile(vsCode, strlen(vsCode), "VS_Quad", nullptr, nullptr, "VS", "vs_4_0", 0, 0, &vsBlob, &errBlob))) {
        if (errBlob) std::cerr << "VS Compile Error: " << (char*)errBlob->GetBufferPointer() << std::endl;
        return false;
    }
    m_D3DDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_VS);
    vsBlob->Release();

    // 2. Compile PS (Load from file normally, but simpler to embed or load specifically)
    // For MVP, we'll try to load "StereoReprojection.hlsl" relative to DLL or current dir.
    // If fail, we won't have stereo.
    
    // NOTE: For robustness in this code-only env, I will omit the file loading and expect the user to ensure it works,
    // OR I can embed a minimal shader string here too. I'll embed a minimal fallback.
    const char* psCode = R"(
        Texture2D g_Color : register(t0);
        Texture2D<float> g_Depth : register(t1);
        SamplerState g_Sampler : register(s0);
        cbuffer Params : register(b0) { float g_Separation; float g_Convergence; float g_EyeSign; float g_Padding; };
        struct PS_INPUT { float4 Pos : SV_POSITION; float2 Tex : TEXCOORD0; };
        float4 PS(PS_INPUT input) : SV_Target {
            float z = g_Depth.SampleLevel(g_Sampler, input.Tex, 0);
            float shift = g_Separation * (1.0 - z) * g_EyeSign;
            float2 uv = input.Tex + float2(shift, 0);
            if(uv.x<0||uv.x>1) return float4(0,0,0,1);
            return g_Color.Sample(g_Sampler, uv);
        }
    )";

    ID3DBlob* psBlob = nullptr;
    if (FAILED(D3DCompile(psCode, strlen(psCode), "PS_Stereo", nullptr, nullptr, "PS", "ps_4_0", 0, 0, &psBlob, &errBlob))) {
         if (errBlob) std::cerr << "PS Compile Error: " << (char*)errBlob->GetBufferPointer() << std::endl;
        return false;
    }
    m_D3DDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_PS);
    psBlob->Release();

    // 3. Constant Buffer
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(CBParams);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    m_D3DDevice->CreateBuffer(&bd, nullptr, &m_ParamBuffer);

    // 4. Sampler
    D3D11_SAMPLER_DESC sd = {};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    m_D3DDevice->CreateSamplerState(&sd, &m_Sampler);

    m_StereoInitialized = true;
    return true;
}

void OpenXRManager::RenderStereo(ID3D11Texture2D* color, ID3D11Texture2D* depth, ID3D11RenderTargetView* rtv, int eye) {
    if (!m_StereoInitialized) InitStereoResources();
    if (!m_VS || !m_PS) return; // Fallback

    // Setup RT
    m_D3DContext->OMSetRenderTargets(1, &rtv, nullptr);
    
    // Setup Viewport (Assuming full texture)
    D3D11_TEXTURE2D_DESC td;
    color->GetDesc(&td); // Approximation, ideally RTV size
    D3D11_VIEWPORT vp = { 0, 0, (float)td.Width, (float)td.Height, 0, 1 };
    m_D3DContext->RSSetViewports(1, &vp);

    // Update Params
    CBParams params = m_Params;
    params.EyeSign = (eye == 0) ? -1.0f : 1.0f;
    m_D3DContext->UpdateSubresource(m_ParamBuffer, 0, nullptr, &params, 0, 0);

    // Bind Resources
    ID3D11ShaderResourceView* srvs[2] = { nullptr, nullptr };
    
    // Create SRVs on the fly (Performance warning! Should cache)
    // NOTE: This assumes color/depth have BIND_SHADER_RESOURCE. Game backbuffer usually does. Depth might not.
    // If Depth fails creation, we skip.
    m_D3DDevice->CreateShaderResourceView(color, nullptr, &srvs[0]);
    
    // Depth SRV needs correct format (e.g., R32_FLOAT for D32_FLOAT)
    // We try to create default.
    m_D3DDevice->CreateShaderResourceView(depth, nullptr, &srvs[1]);

    m_D3DContext->PSSetShaderResources(0, 2, srvs);
    m_D3DContext->PSSetSamplers(0, 1, &m_Sampler);
    m_D3DContext->PSSetConstantBuffers(0, 1, &m_ParamBuffer);
    
    m_D3DContext->VSSetShader(m_VS, nullptr, 0);
    m_D3DContext->PSSetShader(m_PS, nullptr, 0);
    
    // Draw Full Screen Triangle (3 verts generated by VS)
    m_D3DContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_D3DContext->Draw(3, 0);

    // Cleanup
    ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
    m_D3DContext->PSSetShaderResources(0, 2, nullSRVs);
    if (srvs[0]) srvs[0]->Release();
    if (srvs[1]) srvs[1]->Release();
}
