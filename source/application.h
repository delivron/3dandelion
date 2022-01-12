#pragma once

#include "window.h"
#include "keyboard.h"

#include <memory>
#include <string>
#include <cstdint>

namespace ddn
{

class Application
    : public IWindowListener
{
public:
    Application(std::unique_ptr<Window>&& window);
    Application(const std::wstring& title, uint32_t width, uint32_t height);

    Window& GetWindow();
    const Window& GetWindow() const;

    Keyboard& GetKeyboard();
    const Keyboard& GetKeyboard() const;

    int Run();

private:
    std::unique_ptr<Window> m_window;
    Keyboard m_keyboard;
};

}  // namespace ddn
