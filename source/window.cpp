#include "window.h"

#include <cassert>

namespace
{

ATOM RegisterWindowClass(HINSTANCE instance, const std::wstring& class_name)
{
    WNDCLASSEXW window_class = {};
    window_class.cbSize = sizeof(WNDCLASSEXW);
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = &DefWindowProcW;
    window_class.hInstance = instance;
    window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    window_class.lpszClassName = class_name.c_str();

    ATOM id = RegisterClassExW(&window_class);
    assert(id && "Failed to register window class");
    return id;
}

HWND CreateDefaultWindow(HINSTANCE instance, const std::wstring& class_name, const std::wstring& title, uint32_t width, uint32_t height)
{
    const int screen_width = GetSystemMetrics(SM_CXSCREEN);
    const int screen_height = GetSystemMetrics(SM_CYSCREEN);

    RECT window_rect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
    AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW, FALSE);

    int window_width = window_rect.right - window_rect.left;
    int window_height = window_rect.bottom - window_rect.top;
    int window_x = std::max<int>(0, (screen_width - window_width) / 2);
    int window_y = std::max<int>(0, (screen_height - window_height) / 2);

    HWND hwnd = CreateWindowW(
        class_name.c_str(),
        title.c_str(),
        WS_OVERLAPPEDWINDOW,
        window_x,
        window_y,
        window_width,
        window_height,
        nullptr,
        nullptr,
        instance,
        nullptr
    );
    assert(hwnd && "Failed to create window");
    return hwnd;
}

}  // namespace

namespace ddn
{

Window::Window(const std::wstring& title, uint32_t width, uint32_t height)
{
    const HINSTANCE instance = GetModuleHandleW(nullptr);
    const std::wstring class_name = L"DandelionWindowClass";
    RegisterWindowClass(instance, class_name);
    m_handle = CreateDefaultWindow(instance, class_name, title, width, height);
}

HWND Window::GetHandle() const
{
    return m_handle;
}

void Window::Show()
{
    ShowWindow(m_handle, SW_SHOWDEFAULT);
}

}  // namespace ddn
