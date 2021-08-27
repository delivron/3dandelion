#include "application.h"

#include <wrl.h>
#include <d3dx12.h>
#include <dxgi1_4.h>
#include <Windows.h>

#include <array>
#include <stdexcept>
#include <algorithm>

using namespace Microsoft::WRL;

void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr)) {
        throw std::runtime_error("Invalid return value");
    }
}

class DandelionApp : public ddn::Application
{
public:
    DandelionApp(const std::wstring& title, uint32_t width, uint32_t height)
        : ddn::Application(title, width, height)
    {
        ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&m_factory)));
        ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));

        D3D12_COMMAND_QUEUE_DESC queue_desc = {};
        queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        ThrowIfFailed(m_device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&m_queue)));
        ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_command_allocator)));
        ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_command_allocator.Get(), nullptr, IID_PPV_ARGS(&m_command_list)));
        ThrowIfFailed(m_command_list->Close());

        const ddn::Window& window = GetWindow();
        const HWND window_handle = window.GetHandle();

        ComPtr<IDXGISwapChain1> swap_chain;
        DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
        swap_chain_desc.BufferCount = s_back_buffer_count;
        swap_chain_desc.Width = window.GetWidth();
        swap_chain_desc.Height = window.GetHeight();
        swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swap_chain_desc.SampleDesc.Count = 1;
        ThrowIfFailed(m_factory->CreateSwapChainForHwnd(m_queue.Get(), window_handle, &swap_chain_desc, nullptr, nullptr, &swap_chain));
        ThrowIfFailed(swap_chain.As(&m_swap_chain));

        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = s_back_buffer_count;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        ThrowIfFailed(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_descriptor_heap)));

        m_rtv_descriptor_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        UpdateBackBufferViews();

        ThrowIfFailed(m_device->CreateFence(m_fence_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));

        m_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    }

    void OnResize(uint32_t width, uint32_t height) override
    {
        WaitForGpu();

        width = std::max<uint32_t>(1, width);
        height = std::max<uint32_t>(1, height);

        for (auto& back_buffer : m_back_buffers) {
            back_buffer.Reset();
        }

        DXGI_SWAP_CHAIN_DESC1 swap_chian_desc = {};
        ThrowIfFailed(m_swap_chain->GetDesc1(&swap_chian_desc));
        ThrowIfFailed(m_swap_chain->ResizeBuffers(swap_chian_desc.BufferCount, width, height, swap_chian_desc.Format, swap_chian_desc.Flags));

        UpdateBackBufferViews();

        m_buffer_index = m_swap_chain->GetCurrentBackBufferIndex();
    }

    void OnUpdate() override
    {
        m_color_value += s_color_delta;
        if (m_color_value > 1.0f) {
            m_color_value = 0.0f;
        }

        ComPtr<ID3D12Resource> back_buffer = m_back_buffers[m_buffer_index];

        ThrowIfFailed(m_command_allocator->Reset());
        ThrowIfFailed(m_command_list->Reset(m_command_allocator.Get(), nullptr));

        auto barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(back_buffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_command_list->ResourceBarrier(1, &barrier1);

        const float color[4] = { m_color_value, m_color_value, m_color_value, 1.0f };
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(m_descriptor_heap->GetCPUDescriptorHandleForHeapStart(), m_buffer_index, m_rtv_descriptor_size);
        m_command_list->ClearRenderTargetView(rtv_handle, color, 0, nullptr);

        auto barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(back_buffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        m_command_list->ResourceBarrier(1, &barrier2);

        m_command_list->Close();

        ID3D12CommandList* command_lists[] = { m_command_list.Get() };
        m_queue->ExecuteCommandLists(static_cast<UINT>(std::size(command_lists)), command_lists);

        ThrowIfFailed(m_swap_chain->Present(0, 0));

        WaitForGpu();
    }

private:
    void WaitForGpu()
    {
        ++m_fence_value;

        ThrowIfFailed(m_queue->Signal(m_fence.Get(), m_fence_value));

        if (m_fence->GetCompletedValue() < m_fence_value) {
            ThrowIfFailed(m_fence->SetEventOnCompletion(m_fence_value, m_event));
            WaitForSingleObject(m_event, INFINITE);
        }

        m_buffer_index = m_swap_chain->GetCurrentBackBufferIndex();
    }

    void UpdateBackBufferViews()
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(m_descriptor_heap->GetCPUDescriptorHandleForHeapStart());
        for (UINT i = 0; i < s_back_buffer_count; ++i) {
            ThrowIfFailed(m_swap_chain->GetBuffer(i, IID_PPV_ARGS(&m_back_buffers[i])));
            m_device->CreateRenderTargetView(m_back_buffers[i].Get(), nullptr, rtv_handle);
            rtv_handle.Offset(1, m_rtv_descriptor_size);
        }
    }

private:
    static constexpr UINT s_back_buffer_count = 2;
    static constexpr float s_color_delta = 0.01f;

    ComPtr<IDXGIFactory4> m_factory;
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12CommandQueue> m_queue;
    ComPtr<ID3D12CommandAllocator> m_command_allocator;
    ComPtr<ID3D12GraphicsCommandList> m_command_list;
    ComPtr<IDXGISwapChain3> m_swap_chain;

    ComPtr<ID3D12DescriptorHeap> m_descriptor_heap;
    UINT m_rtv_descriptor_size = 0;

    std::array<ComPtr<ID3D12Resource>, s_back_buffer_count> m_back_buffers;
    UINT m_buffer_index = 0;

    ComPtr<ID3D12Fence> m_fence;
    uint64_t m_fence_value = 0;
    HANDLE m_event = nullptr;

    float m_color_value = 0.0;
};

int main()
{
    DandelionApp app(L"3Dandelion", 800, 600);
    return app.Run();
}
