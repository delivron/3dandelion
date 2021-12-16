#include "swap-chain.h"

#include "utils.h"
#include "window.h"
#include "command-queue.h"

#include <exception>

using namespace Microsoft::WRL;

namespace ddn
{

SwapChain::SwapChain(IDXGIFactory5& factory, CommandQueue& command_queue, Window& window, uint32_t back_buffer_count)
    : m_command_queue(command_queue)
    , m_fence(*GetDevice(static_cast<ID3D12CommandQueue&>(command_queue)).Get())
{
    const auto& desc = command_queue->GetDesc();
    if (desc.Type != D3D12_COMMAND_LIST_TYPE_DIRECT) {
        throw std::invalid_argument("Expected direct command queue");
    }

    if (back_buffer_count < 2 || back_buffer_count > DXGI_MAX_SWAP_CHAIN_BUFFERS) {
        throw std::invalid_argument("Expected back buffer count from 2 to DXGI_MAX_SWAP_CHAIN_BUFFERS");
    }

    m_back_buffer_count = back_buffer_count;

    BOOL allow_tearing = false;
    HRESULT hr = factory.CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allow_tearing, sizeof(allow_tearing));
    m_is_tearing_supported = SUCCEEDED(hr) && allow_tearing;

    ComPtr<IDXGISwapChain1> swap_chain;
    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
    swap_chain_desc.BufferCount = back_buffer_count;
    swap_chain_desc.Width = window.GetWidth();
    swap_chain_desc.Height = window.GetHeight();
    swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_chain_desc.SampleDesc.Count = 1;
    swap_chain_desc.Flags = m_is_tearing_supported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

    const HWND window_handle = window.GetHandle();
    ValidateResult(factory.CreateSwapChainForHwnd(command_queue, window_handle, &swap_chain_desc, nullptr, nullptr, &swap_chain));
    ValidateResult(factory.MakeWindowAssociation(window_handle, DXGI_MWA_NO_ALT_ENTER));
    ValidateResult(swap_chain.As(&m_instance));

    UpdateBackBuffers();
}

uint32_t SwapChain::GetBackBufferCount() const
{
    return m_back_buffer_count;
}

uint32_t SwapChain::GetCurrentBackBufferIndex() const
{
    return m_instance->GetCurrentBackBufferIndex();
}

ComPtr<ID3D12Resource> SwapChain::GetCurrentBackBuffer()
{
    const auto index = GetCurrentBackBufferIndex();
    return m_back_buffers[index].resource;
}

ComPtr<ID3D12Resource> SwapChain::GetBackBuffer(uint32_t index)
{
    if (index >= m_back_buffers.size()) {
        return nullptr;
    }
    return m_back_buffers[index].resource;
}

void SwapChain::Resize(uint32_t width, uint32_t height)
{
    m_command_queue.Flush();

    ReleaseBackBuffers();

    DXGI_SWAP_CHAIN_DESC1 swap_chian_desc = {};
    ValidateResult(m_instance->GetDesc1(&swap_chian_desc));
    ValidateResult(m_instance->ResizeBuffers(swap_chian_desc.BufferCount, width, height, swap_chian_desc.Format, swap_chian_desc.Flags));

    UpdateBackBuffers();
}

void SwapChain::Present()
{
    auto index = GetCurrentBackBufferIndex();
    m_back_buffers[index].fence_value = m_fence.Signal(m_command_queue);

    UINT present_flags = m_is_tearing_supported ? DXGI_PRESENT_ALLOW_TEARING : 0;
    m_instance->Present(0, present_flags);

    index = GetCurrentBackBufferIndex();
    m_fence.Wait(m_back_buffers[index].fence_value);
}

void SwapChain::ReleaseBackBuffers()
{
    for (auto& back_buffer : m_back_buffers) {
        back_buffer.resource.Reset();
        back_buffer.fence_value = 0;
    }
}

void SwapChain::UpdateBackBuffers()
{
    m_back_buffers.resize(m_back_buffer_count);
    for (uint32_t i = 0; i < m_back_buffer_count; ++i) {
        ValidateResult(m_instance->GetBuffer(i, IID_PPV_ARGS(&m_back_buffers[i].resource)));
    }
}

}  // namespace ddn
