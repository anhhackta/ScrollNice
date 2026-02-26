#pragma once
#include <windows.h>
#include <vector>
#include "../../core/Zone.h"
#include "../../core/StateMachine.h"

namespace sn {

class WinOverlay {
public:
    bool Create(HINSTANCE hInst);
    void Destroy();

    // Update zone visuals
    void UpdateZones(const std::vector<Zone>& zones, AppState state, const Zone* activeZone);

    // Toggle edit mode (enable/disable click-through)
    void SetEditMode(bool edit);

    void Show();
    void Hide();

    HWND Handle() const { return hwnd_; }

private:
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    void Paint();

    HWND hwnd_ = nullptr;
    std::vector<Zone> zones_;
    AppState state_ = AppState::Idle;
    const Zone* activeZone_ = nullptr;
    bool editMode_ = false;
    double hoverAlpha_ = 0.15;
    double activeAlpha_ = 0.25;
};

} // namespace sn
