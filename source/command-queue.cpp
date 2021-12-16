#include "command-queue.h"
#include "utils.h"

#include <algorithm>

namespace ddn
{

CommandQueue::CommandQueue(ID3D12Device& device, D3D12_COMMAND_LIST_TYPE type)
    : m_fence(device)
{
    D3D12_COMMAND_QUEUE_DESC queue_desc = {};
    queue_desc.Type = type;
    ValidateResult(device.CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&m_instance)));
}

ID3D12CommandQueue* CommandQueue::operator ->()
{
    return m_instance.Get();
}

CommandQueue::operator ID3D12CommandQueue*()
{
    return m_instance.Get();
}

CommandQueue::operator ID3D12CommandQueue&()
{
    return *m_instance.Get();
}

void CommandQueue::Clear() noexcept
{
    std::lock_guard guard(m_mutex);
    m_command_lists.clear();
}

void CommandQueue::Add(Microsoft::WRL::ComPtr<ID3D12CommandList> command_list) noexcept
{
    if (!command_list) {
        return;
    }

    std::lock_guard guard(m_mutex);
    m_command_lists.emplace_back(std::move(command_list));
}

void CommandQueue::Execute()
{
    std::lock_guard guard(m_mutex);

    if (m_command_lists.empty()) {
        return;
    }

    std::vector<ID3D12CommandList*> command_lists(m_command_lists.size());
    std::transform(m_command_lists.cbegin(), m_command_lists.cend(), command_lists.begin(), [](const Microsoft::WRL::ComPtr<ID3D12CommandList>& command_list) {
        return command_list.Get();
    });

    m_instance->ExecuteCommandLists(static_cast<UINT>(command_lists.size()), command_lists.data());
}

uint64_t CommandQueue::Signal(Fence& fence)
{
    return fence.Signal(*this);
}

void CommandQueue::Wait(Fence& fence)
{
    fence.Wait(*this);
}

void CommandQueue::Flush()
{
    Signal(m_fence);
    Wait(m_fence);
    m_fence.Wait();
}

}  // namespace ddn
