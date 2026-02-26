#include "WinMouseHook.h"

namespace sn {

WinMouseHook& WinMouseHook::Instance() {
    static WinMouseHook inst;
    return inst;
}

bool WinMouseHook::Install(MouseHookCallback cb) {
    callback_ = cb;
    hook_ = SetWindowsHookExW(WH_MOUSE_LL, LowLevelMouseProc, GetModuleHandleW(nullptr), 0);
    return hook_ != nullptr;
}

void WinMouseHook::Uninstall() {
    if (hook_) {
        UnhookWindowsHookEx(hook_);
        hook_ = nullptr;
    }
    callback_ = nullptr;
}

LRESULT CALLBACK WinMouseHook::LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        auto* data = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
        auto& inst = Instance();
        if (inst.callback_) {
            bool eat = inst.callback_(data->pt, (DWORD)wParam, data);
            if (eat) return 1; // block the event
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

} // namespace sn
