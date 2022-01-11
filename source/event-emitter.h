#pragma once

#include <list>
#include <mutex>

namespace ddn
{

template <typename Listener>
class EventEmitter
{
public:
    virtual ~EventEmitter() = default;

    void Subscribe(Listener* listener)
    {
        if (!listener) {
            return;
        }

        std::lock_guard guard(m_mutex);
        m_listeners.push_front(listener);
    }

    void Unsubscribe(Listener* listener)
    {
        std::lock_guard guard(m_mutex);
        m_listeners.remove(listener);
    }

protected:
    template <typename Callback, typename ...Args>
    void Notify(Callback&& callback, Args&&... args)
    {
        std::lock_guard guard(m_mutex);
        for (auto* listener : m_listeners) {
            (listener->*callback)(args...);
        }
    }

private:
    std::mutex m_mutex;
    std::forward_list<Listener*> m_listeners;
};

}  // namespace ddn
