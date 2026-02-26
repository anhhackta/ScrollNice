#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <functional>

namespace sn {

struct HotkeyBinding {
    int id;
    UINT modifiers;
    UINT vk;
};

class WinHotkeys {
public:
    using HotkeyCallback = std::function<void(int id)>;

    // Register hotkeys for a given window. Returns number registered.
    int Register(HWND hwnd, const std::string& toggleEnabled,
                 const std::string& toggleEdit,
                 const std::string& toggleZone,
                 const std::string& toggleWheel,
                 HotkeyCallback cb);

    void Unregister(HWND hwnd);

    void HandleMessage(WPARAM wParam);

    enum HotkeyId {
        HK_TOGGLE_ENABLED = 1,
        HK_TOGGLE_EDIT    = 2,
        HK_TOGGLE_ZONE    = 3,
        HK_TOGGLE_WHEEL   = 4
    };

private:
    static bool ParseHotkey(const std::string& str, UINT& modifiers, UINT& vk);
    HotkeyCallback callback_;
    std::vector<HotkeyBinding> bindings_;
};

} // namespace sn
