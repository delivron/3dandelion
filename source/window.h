#pragma once

#include <Windows.h>

#include <string>
#include <cstdint>

namespace ddn
{

class Window
{
public:
    Window(const std::wstring& title, uint32_t width, uint32_t height);

    HWND GetHandle() const;
    uint32_t GetWidth() const;
    uint32_t GetHeight() const;

    void Show();
    void Hide();

private:
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    HWND m_handle = nullptr;
};

}  // namespace ddn
