#pragma once

#include <Windows.h>

#include <atomic>
#include <string>
#include <cstdint>

namespace ddn
{

class IWindowListener
{
public:
    virtual void OnResize(uint32_t width, uint32_t height) {}
    virtual void OnUpdate() {}
    virtual void OnDestroy() {}
};

class Window
{
public:
    Window(const std::wstring& title, uint32_t width, uint32_t height);

    HWND GetHandle() const;
    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    void Show();
    void Hide();
    void Subscribe(IWindowListener* listener);
    void Unsubscribe();

private:
    std::wstring Register();
    HWND Create(const std::wstring& title, uint32_t width, uint32_t height);
    bool OnMessage(UINT message, WPARAM w_param, LPARAM l_param);

    static LRESULT CALLBACK ProcessMessage(HWND handle, UINT message, WPARAM w_param, LPARAM l_param);

private:
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    HWND m_handle = nullptr;
    std::atomic<IWindowListener*> m_listener = nullptr;
};

}  // namespace ddn
