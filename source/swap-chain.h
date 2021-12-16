#pragma once

#include "fence.h"

#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_5.h>

#include <vector>
#include <cstdint>

namespace ddn
{

class Window;
class CommandQueue;

class SwapChain
{
public:
    SwapChain(IDXGIFactory5& factory, CommandQueue& command_queue, Window& window, uint32_t back_buffer_count);

    SwapChain(const SwapChain& other) = delete;
    SwapChain& operator =(const SwapChain& other) = delete;

    uint32_t GetBackBufferCount() const;
    uint32_t GetCurrentBackBufferIndex() const;
    Microsoft::WRL::ComPtr<ID3D12Resource> GetCurrentBackBuffer();
    Microsoft::WRL::ComPtr<ID3D12Resource> GetBackBuffer(uint32_t index);

    void Resize(uint32_t width, uint32_t height);
    void Present();

private:
    void ReleaseBackBuffers();
    void UpdateBackBuffers();

private:
    struct BackBuffer
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        uint64_t fence_value = 0;
    };

private:
    CommandQueue& m_command_queue;
    Fence m_fence;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> m_instance;
    std::vector<BackBuffer> m_back_buffers;
    bool m_is_tearing_supported = false;
    uint32_t m_back_buffer_count = 0;
};

}  // namespace ddn
