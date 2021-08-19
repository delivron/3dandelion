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

    void Show();

private:
    HWND m_handle = nullptr;
};

}  // namespace ddn
