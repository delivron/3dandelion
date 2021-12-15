#pragma once

#include <wrl.h>
#include <d3d12.h>

#include <atomic>

namespace ddn
{

class Fence
{
public:
    Fence(ID3D12Device& device);
    ~Fence();

    Fence(const Fence& other) = delete;
    Fence& operator=(const Fence& other) = delete;

    void Signal();
    void Signal(ID3D12CommandQueue& command_queue);

    void Wait();
    void Wait(ID3D12CommandQueue& command_queue);

private:
    Microsoft::WRL::ComPtr<ID3D12Fence> m_instance;
    std::atomic_uint64_t m_signaled_value = 0;
    HANDLE m_event = INVALID_HANDLE_VALUE;
};

}  // namespace ddn
