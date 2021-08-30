#include "window.h"

#include <cassert>

namespace ddn
{

Window::Window(const std::wstring& title, uint32_t width, uint32_t height)
    : m_window_class_name(L"DandelionWindowClass" + std::to_wstring(reinterpret_cast<std::uintptr_t>(this)))
    , m_width(width)
    , m_height(height)
    , m_handle(Create(title, width, height))
{
}

Window::~Window()
{
    const HINSTANCE instance = GetModuleHandle(nullptr);
    UnregisterClass(m_window_class_name.c_str(), instance);
    DestroyWindow(m_handle);
}

HWND Window::GetHandle() const
{
    return m_handle;
}

uint32_t Window::GetWidth() const
{
    return m_width;
}

uint32_t Window::GetHeight() const
{
    return m_height;
}

void Window::Show()
{
    ShowWindow(m_handle, SW_SHOWDEFAULT);
}

void Window::Hide()
{
    ShowWindow(m_handle, SW_HIDE);
}

void Window::Subscribe(IWindowListener* listener)
{
    m_listener.store(listener);
}

void Window::Unsubscribe()
{
    m_listener.store(nullptr);
}

HWND Window::Create(const std::wstring& title, uint32_t width, uint32_t height)
{
    const int screen_width = GetSystemMetrics(SM_CXSCREEN);
    const int screen_height = GetSystemMetrics(SM_CYSCREEN);

    RECT window_rect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
    AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW, false);

    int window_width = window_rect.right - window_rect.left;
    int window_height = window_rect.bottom - window_rect.top;
    int window_x = std::max<int>(0, (screen_width - window_width) / 2);
    int window_y = std::max<int>(0, (screen_height - window_height) / 2);

    const HINSTANCE instance = GetModuleHandle(nullptr);
    WNDCLASSEXW window_class = {};
    window_class.cbSize = sizeof(WNDCLASSEXW);
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = &OnMessage;
    window_class.hInstance = instance;
    window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    window_class.lpszClassName = m_window_class_name.c_str();
    RegisterClassEx(&window_class);

    HWND hwnd = CreateWindow(
        m_window_class_name.c_str(),
        title.c_str(),
        WS_OVERLAPPEDWINDOW,
        window_x,
        window_y,
        window_width,
        window_height,
        nullptr,
        nullptr,
        instance,
        this
    );
    assert(hwnd && "Failed to create window");
    return hwnd;
}

bool Window::ProcessMessage(UINT message, WPARAM w_param, LPARAM l_param)
{
    IWindowListener default_listener = {};
    IWindowListener* listener = m_listener.load();
    if (!listener) {
        listener = &default_listener;
    }

    switch (message)
    {
    case WM_SIZE:
    {
        RECT client_rect = {};
        GetClientRect(m_handle, &client_rect);
        m_width = client_rect.right - client_rect.left;
        m_height = client_rect.bottom - client_rect.top;
        listener->OnResize(m_width, m_height);
        break;
    }
    case WM_PAINT:
    {
        listener->OnUpdate();
        break;
    }
    case WM_DESTROY:
    {
        listener->OnDestroy();
        PostQuitMessage(0);
        break;
    }
    default:
        return false;
    }

    return true;
}

LRESULT CALLBACK Window::OnMessage(HWND handle, UINT message, WPARAM w_param, LPARAM l_param)
{
    if (message == WM_CREATE) {
        LPCREATESTRUCT creation_desc = reinterpret_cast<LPCREATESTRUCT>(l_param);
        SetWindowLongPtr(handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(creation_desc->lpCreateParams));
        return 0;
    }

    LONG_PTR ptr = GetWindowLongPtr(handle, GWLP_USERDATA);
    auto* window = reinterpret_cast<Window*>(ptr);
    if (!window) {
        return DefWindowProc(handle, message, w_param, l_param);
    }

    bool is_processed= window->ProcessMessage(message, w_param, l_param);
    if (is_processed) {
        return 0;
    }

    return DefWindowProc(handle, message, w_param, l_param);
}

}  // namespace ddn
