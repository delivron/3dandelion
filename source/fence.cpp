#include "fence.h"
#include "utils.h"
#include "command-queue.h"

namespace ddn
{

Fence::Fence(ID3D12Device& device)
    : m_event(CreateEvent(nullptr, false, false, nullptr))
{
    ValidateResult(device.CreateFence(m_signaled_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_instance)));
}

Fence::~Fence()
{
    if (m_event && m_event != INVALID_HANDLE_VALUE) {
        CloseHandle(m_event);
    }
}

uint64_t Fence::Signal()
{
    auto value = ++m_signaled_value;
    ValidateResult(m_instance->Signal(value));
    return value;
}

uint64_t Fence::Signal(CommandQueue& command_queue)
{
    auto value = ++m_signaled_value;
    ValidateResult(command_queue->Signal(m_instance.Get(), value));
    return value;
}

void Fence::Wait()
{
    Wait(m_signaled_value);
}

void Fence::Wait(uint64_t value)
{
    const auto current_value = m_instance->GetCompletedValue();
    if (current_value >= value) {
        return;
    }

    ValidateResult(m_instance->SetEventOnCompletion(value, m_event));
    WaitForSingleObject(m_event, INFINITE);
}

void Fence::Wait(CommandQueue& command_queue)
{
    command_queue->Wait(m_instance.Get(), m_signaled_value);
}

}  // namespace ddn
