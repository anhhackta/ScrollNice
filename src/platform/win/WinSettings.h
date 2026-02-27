#pragma once
#include <windows.h>
#include <string>
#include <functional>
#include "../../core/Config.h"

namespace sn {

using SettingsCallback = std::function<void(const AppConfig&)>;

class WinSettings {
public:
    // Show modal settings dialog. Calls back with updated config on OK.
    void Show(HINSTANCE hInst, HWND parent, AppConfig& cfg, SettingsCallback onSave);

private:
    static INT_PTR CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
    void InitControls(HWND dlg);
    void ReadControls(HWND dlg);

    AppConfig* cfg_ = nullptr;
    SettingsCallback onSave_;
    HWND dlg_ = nullptr;
};

} // namespace sn
