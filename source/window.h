#pragma once

#include "event-emitter.h"

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
    virtual void OnRender() {}
    virtual void OnDestroy() {}
    virtual void OnKeyDown(uint8_t key_code) {}
    virtual void OnKeyUp(uint8_t key_code) {}
};

class Window
    : public EventEmitter<IWindowListener>
{
public:
    Window(const std::wstring& title, uint32_t width, uint32_t height);
    ~Window();

    HWND GetHandle() const;
    uint32_t GetWidth() const;
    uint32_t GetHeight() const;

    void Show();
    void Hide();

private:
    HWND Create(const std::wstring& title, uint32_t width, uint32_t height);
    bool ProcessMessage(UINT message, WPARAM w_param, LPARAM l_param);

    static LRESULT CALLBACK OnMessage(HWND handle, UINT message, WPARAM w_param, LPARAM l_param);

private:
    std::wstring m_window_class_name;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    HWND m_handle = nullptr;
};

}  // namespace ddn
