#pragma once

#include "window.h"

#include <vector>

namespace ddn
{

class Keyboard
    : public IWindowListener
{
public:
    Keyboard();

    bool IsKeyPressed(uint8_t key_code) const;

    void OnKeyDown(uint8_t key_code) override;
    void OnKeyUp(uint8_t key_code) override;

private:
    std::vector<bool> m_key_to_pressed_state;
};

}  // namespace ddn
