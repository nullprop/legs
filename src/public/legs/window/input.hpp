#pragma once

#include <glm/vec2.hpp>
#include <SDL_scancode.h>

namespace legs
{

enum Key : unsigned int
{
    KEY_NONE,

    KEY_MOVE_FORWARD,
    KEY_MOVE_BACK,
    KEY_MOVE_RIGHT,
    KEY_MOVE_LEFT,
    KEY_MOVE_UP,
    KEY_MOVE_DOWN,

    KEY_MOUSE_GRAB,

    KEY_WINDOW_DEBUG,
    KEY_WINDOW_DEMO,

    KEY_MAX,
};

static constexpr unsigned long long KeyToFlag(Key k)
{
    return 1 << static_cast<unsigned int>(k);
}

class InputSettings
{
  public:
    InputSettings()
    {
        for (unsigned int i = 0; i < SDL_NUM_SCANCODES; i++)
        {
            m_sdlKeyMap[i] = Key::KEY_NONE;
        }

        ApplyDefaults();
    }

    void ApplyDefaults()
    {
        m_sdlKeyMap[static_cast<unsigned int>(SDL_SCANCODE_W)]     = Key::KEY_MOVE_FORWARD;
        m_sdlKeyMap[static_cast<unsigned int>(SDL_SCANCODE_A)]     = Key::KEY_MOVE_LEFT;
        m_sdlKeyMap[static_cast<unsigned int>(SDL_SCANCODE_S)]     = Key::KEY_MOVE_BACK;
        m_sdlKeyMap[static_cast<unsigned int>(SDL_SCANCODE_D)]     = Key::KEY_MOVE_RIGHT;
        m_sdlKeyMap[static_cast<unsigned int>(SDL_SCANCODE_SPACE)] = Key::KEY_MOVE_UP;
        m_sdlKeyMap[static_cast<unsigned int>(SDL_SCANCODE_LCTRL)] = Key::KEY_MOVE_DOWN;

        m_sdlKeyMap[static_cast<unsigned int>(SDL_SCANCODE_F1)] = Key::KEY_MOUSE_GRAB;

        m_sdlKeyMap[static_cast<unsigned int>(SDL_SCANCODE_F2)] = Key::KEY_WINDOW_DEBUG;
        m_sdlKeyMap[static_cast<unsigned int>(SDL_SCANCODE_F3)] = Key::KEY_WINDOW_DEMO;
    }

    Key GetKeyFromSDL(unsigned int scan)
    {
        return m_sdlKeyMap[scan];
    }

  private:
    Key m_sdlKeyMap[SDL_NUM_SCANCODES];
};

struct WindowInput
{

    bool wantsQuit;
    bool wantsResize;

    unsigned long long keyFlags;

    glm::ivec2 mouse;
    glm::ivec2 scroll;

    WindowInput()
    {
        Clear(true);
    }

    void Clear(bool clearKeys = false)
    {
        wantsQuit   = false;
        wantsResize = false;

        // Generally don't want to do this,
        // since window only tracks key up / down events.
        if (clearKeys)
        {
            keyFlags = 0;
        }

        mouse.x  = 0;
        mouse.y  = 0;
        scroll.x = 0;
        scroll.y = 0;
    }

    void Aggregate(const WindowInput& other)
    {
        wantsQuit |= other.wantsQuit;
        wantsResize |= other.wantsResize;

        keyFlags |= other.keyFlags;

        mouse += other.mouse;
        scroll += other.scroll;
    }

    bool HasKey(Key key)
    {
        return (keyFlags & KeyToFlag(key)) == KeyToFlag(key);
    }

    void KeyDown(Key key)
    {
        keyFlags |= KeyToFlag(key);
    }

    void KeyUp(Key key)
    {
        keyFlags &= ~KeyToFlag(key);
    }
};
}; // namespace legs
