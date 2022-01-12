#include "keyboard.h"

#include <climits>

namespace ddn
{

Keyboard::Keyboard()
    : m_key_to_pressed_state(std::numeric_limits<uint8_t>::max() + 1, false)
{}

bool Keyboard::IsKeyPressed(uint8_t key_code) const
{
    return m_key_to_pressed_state[key_code];
}

void Keyboard::OnKeyDown(uint8_t key_code)
{
    m_key_to_pressed_state[key_code] = true;
}

void Keyboard::OnKeyUp(uint8_t key_code)
{
    m_key_to_pressed_state[key_code] = false;
}

}  // namespace ddn
