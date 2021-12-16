#pragma once

#include "fence.h"

#include <wrl.h>
#include <d3d12.h>

#include <mutex>
#include <vector>

namespace ddn
{

class CommandQueue
{
public:
    CommandQueue(ID3D12Device& device, D3D12_COMMAND_LIST_TYPE type);

    CommandQueue(const CommandQueue& other) = delete;
    CommandQueue& operator =(const CommandQueue& other) = delete;

    ID3D12CommandQueue* operator ->();
    operator ID3D12CommandQueue*();
    operator ID3D12CommandQueue&();

    void Clear() noexcept;
    void Add(Microsoft::WRL::ComPtr<ID3D12CommandList> command_list) noexcept;
    void Execute();
    void Flush();

private:
    std::mutex m_mutex;
    Fence m_fence;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_instance;
    std::vector<Microsoft::WRL::ComPtr<ID3D12CommandList>> m_command_lists;
};

}  // namespace ddn
