#include "fence.h"
#include "utils.h"

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

void Fence::Signal()
{
    ++m_signaled_value;
    ValidateResult(m_instance->Signal(m_signaled_value));
}

void Fence::Signal(ID3D12CommandQueue& command_queue)
{
    ++m_signaled_value;
    ValidateResult(command_queue.Signal(m_instance.Get(), m_signaled_value));
}

void Fence::Wait()
{
    const auto signaled_value = m_signaled_value.load();
    const auto current_value = m_instance->GetCompletedValue();
    if (current_value >= signaled_value) {
        return;
    }

    ValidateResult(m_instance->SetEventOnCompletion(signaled_value, m_event));
    WaitForSingleObject(m_event, INFINITE);
}


void Fence::Wait(ID3D12CommandQueue& command_queue)
{
    command_queue.Wait(m_instance.Get(), m_signaled_value);
}

}  // namespace ddn
