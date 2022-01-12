#include "application.h"

namespace ddn
{

Application::Application(std::unique_ptr<Window>&& window)
    : m_window(std::move(window))
{
    m_window->Subscribe(&m_keyboard);
    m_window->Subscribe(this);
    m_window->Show();
}

Application::Application(const std::wstring& title, uint32_t width, uint32_t height)
    : Application(std::make_unique<Window>(title, width, height))
{
}

Window& Application::GetWindow()
{
    return *m_window;
}

const Window& Application::GetWindow() const
{
    return *m_window;
}

Keyboard& Application::GetKeyboard()
{
    return m_keyboard;
}

const Keyboard& Application::GetKeyboard() const
{
    return m_keyboard;
}

int Application::Run()
{
    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return static_cast<char>(msg.wParam);
}

}  // namespace ddn
