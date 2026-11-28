#pragma once
// Must match VulkanEngine.h: GLFW only exposes its Vulkan API if this is defined before its first include
#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>
#include <array>

// Per-frame keyboard/mouse snapshot with edge detection.
// Call Update() once per frame after polling events, then query Pressed()/Released()
// instead of keeping a "wasDown" bool per key.
class InputState {
public:
    void Update(GLFWwindow *window) {
        m_PrevKeys = m_Keys;
        m_PrevMouse = m_Mouse;
        for (int k = GLFW_KEY_SPACE; k <= GLFW_KEY_LAST; k++)
            m_Keys[k] = glfwGetKey(window, k) == GLFW_PRESS;
        for (int b = 0; b <= GLFW_MOUSE_BUTTON_LAST; b++)
            m_Mouse[b] = glfwGetMouseButton(window, b) == GLFW_PRESS;
    }

    bool Down(int key) const { return m_Keys[key]; }
    bool Pressed(int key) const { return m_Keys[key] && !m_PrevKeys[key]; }
    bool Released(int key) const { return !m_Keys[key] && m_PrevKeys[key]; }

    bool MouseDown(int button) const { return m_Mouse[button]; }
    bool MousePressed(int button) const { return m_Mouse[button] && !m_PrevMouse[button]; }
    bool MouseReleased(int button) const { return !m_Mouse[button] && m_PrevMouse[button]; }

private:
    std::array<bool, GLFW_KEY_LAST + 1> m_Keys{}, m_PrevKeys{};
    std::array<bool, GLFW_MOUSE_BUTTON_LAST + 1> m_Mouse{}, m_PrevMouse{};
};
