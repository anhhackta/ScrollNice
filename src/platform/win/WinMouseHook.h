#pragma once
#include <windows.h>
#include <functional>

namespace sn {

// Callback: (POINT cursorPos, DWORD mouseMsg) → return true to eat the event
using MouseHookCallback = std::function<bool(POINT pt, DWORD msg, MSLLHOOKSTRUCT* data)>;

class WinMouseHook {
public:
    static WinMouseHook& Instance();

    bool Install(MouseHookCallback cb);
    void Uninstall();
    bool IsInstalled() const { return hook_ != nullptr; }


private:
    WinMouseHook() = default;
    static LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam);

    HHOOK hook_ = nullptr;
    MouseHookCallback callback_;
};

} // namespace sn
