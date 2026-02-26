#include "WinHotkeys.h"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace sn {

static std::string ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c){ return (char)std::tolower(c); });
    return s;
}

bool WinHotkeys::ParseHotkey(const std::string& str, UINT& modifiers, UINT& vk) {
    modifiers = 0;
    vk = 0;

    std::istringstream ss(str);
    std::string token;
    std::vector<std::string> parts;
    while (std::getline(ss, token, '+')) {
        // Trim
        while (!token.empty() && token.front() == ' ') token.erase(token.begin());
        while (!token.empty() && token.back() == ' ') token.pop_back();
        parts.push_back(ToLower(token));
    }

    for (auto& p : parts) {
        if (p == "ctrl" || p == "control") {
            modifiers |= MOD_CONTROL;
        } else if (p == "alt") {
            modifiers |= MOD_ALT;
        } else if (p == "shift") {
            modifiers |= MOD_SHIFT;
        } else if (p == "win") {
            modifiers |= MOD_WIN;
        } else if (p.size() == 1 && std::isalpha((unsigned char)p[0])) {
            vk = (UINT)std::toupper((unsigned char)p[0]);
        } else if (p.size() == 1 && std::isdigit((unsigned char)p[0])) {
            vk = (UINT)p[0];
        } else if (p == "f1")  vk = VK_F1;
        else if (p == "f2")    vk = VK_F2;
        else if (p == "f3")    vk = VK_F3;
        else if (p == "f4")    vk = VK_F4;
        else if (p == "f5")    vk = VK_F5;
        else if (p == "f6")    vk = VK_F6;
        else if (p == "f7")    vk = VK_F7;
        else if (p == "f8")    vk = VK_F8;
        else if (p == "f9")    vk = VK_F9;
        else if (p == "f10")   vk = VK_F10;
        else if (p == "f11")   vk = VK_F11;
        else if (p == "f12")   vk = VK_F12;
    }

    return vk != 0;
}

int WinHotkeys::Register(HWND hwnd, const std::string& toggleEnabled,
                          const std::string& toggleEdit,
                          const std::string& toggleZone,
                          const std::string& toggleWheel,
                          HotkeyCallback cb) {
    callback_ = cb;
    bindings_.clear();
    int count = 0;

    auto tryReg = [&](int id, const std::string& str) {
        UINT mod, vk;
        if (ParseHotkey(str, mod, vk)) {
            if (RegisterHotKey(hwnd, id, mod | MOD_NOREPEAT, vk)) {
                bindings_.push_back({id, mod, vk});
                count++;
            }
        }
    };

    tryReg(HK_TOGGLE_ENABLED, toggleEnabled);
    tryReg(HK_TOGGLE_EDIT,    toggleEdit);
    tryReg(HK_TOGGLE_ZONE,    toggleZone);
    tryReg(HK_TOGGLE_WHEEL,   toggleWheel);

    return count;
}

void WinHotkeys::Unregister(HWND hwnd) {
    for (auto& b : bindings_) {
        UnregisterHotKey(hwnd, b.id);
    }
    bindings_.clear();
}

void WinHotkeys::HandleMessage(WPARAM wParam) {
    int id = (int)wParam;
    if (callback_) callback_(id);
}

} // namespace sn
