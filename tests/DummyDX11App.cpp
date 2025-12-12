#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

struct SimpleVertex {
    XMFLOAT3 Pos;
    XMFLOAT4 Color;
};

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_DESTROY: PostQuitMessage(0); break;
    default: return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "DX11Test", NULL };
    RegisterClassEx(&wc);
    HWND hWnd = CreateWindow("DX11Test", "DX11 Test App", WS_OVERLAPPEDWINDOW, 100, 100, 800, 600, NULL, NULL, hInstance, NULL);

    DXGI_SWAP_CHAIN_DESC sd = { 0 };
    sd.BufferCount = 1;
    sd.BufferDesc.Width = 800;
    sd.BufferDesc.Height = 600;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;

    ID3D11Device* pd3dDevice = nullptr;
    ID3D11DeviceContext* pImmediateContext = nullptr;
    IDXGISwapChain* pSwapChain = nullptr;
    ID3D11RenderTargetView* pRenderTargetView = nullptr;

    D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &sd, &pSwapChain, &pd3dDevice, NULL, &pImmediateContext);

    ID3D11Texture2D* pBackBuffer = NULL;
    pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
    pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &pRenderTargetView);
    pBackBuffer->Release();

    pImmediateContext->OMSetRenderTargets(1, &pRenderTargetView, NULL);

    ShowWindow(hWnd, nCmdShow);
    
    MSG msg = { 0 };
    while (WM_QUIT != msg.message) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            float ClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f }; // Navy Blue
            pImmediateContext->ClearRenderTargetView(pRenderTargetView, ClearColor);
            pSwapChain->Present(0, 0);
        }
    }

    return 0;
}
