#include "window.h"

#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_4.h>

#include <stdexcept>

void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr)) {
        throw std::runtime_error("Invalid return value");
    }
}

int main()
{
    ddn::Window window(L"3Dandelion", 800, 600);
    const HWND window_handle = window.GetHandle();
    window.Show();

    using namespace Microsoft::WRL;
    ComPtr<IDXGIFactory4> factory;
    ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)));

    ComPtr<ID3D12Device> device;
    ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));

    ComPtr<ID3D12CommandQueue> queue;
    {
        D3D12_COMMAND_QUEUE_DESC queue_desc = {};
        queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        ThrowIfFailed(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&queue)));
    }

    ComPtr<IDXGISwapChain1> swap_chain;
    {
        DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
        swap_chain_desc.BufferCount = 2;
        swap_chain_desc.Width = window.GetWidth();
        swap_chain_desc.Height = window.GetHeight();
        swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swap_chain_desc.SampleDesc.Count = 1;
        ThrowIfFailed(factory->CreateSwapChainForHwnd(queue.Get(), window_handle, &swap_chain_desc, nullptr, nullptr, &swap_chain));
    }

    swap_chain->Present(0, 0);

    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessageW(&msg, window_handle, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    return 0;
}
