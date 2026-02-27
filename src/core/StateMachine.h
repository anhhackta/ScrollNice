#pragma once
#include <functional>

namespace sn {

enum class AppState {
    Disabled,
    Idle,
    Edit
};

class StateMachine {
public:
    using StateChangeCallback = std::function<void(AppState oldState, AppState newState)>;

    AppState State() const { return state_; }
    void SetCallback(StateChangeCallback cb) { callback_ = cb; }

    void SetEnabled(bool on) {
        if (on && state_ == AppState::Disabled) TransitionTo(AppState::Idle);
        else if (!on && state_ != AppState::Disabled) TransitionTo(AppState::Disabled);
    }

    void ToggleEnabled() { SetEnabled(state_ == AppState::Disabled); }

    void ToggleEdit() {
        if (state_ == AppState::Edit) TransitionTo(AppState::Idle);
        else if (state_ != AppState::Disabled) TransitionTo(AppState::Edit);
    }

    bool IsEnabled()  const { return state_ != AppState::Disabled; }
    bool IsEditing()  const { return state_ == AppState::Edit; }

private:
    void TransitionTo(AppState ns) {
        if (state_ == ns) return;
        AppState old = state_;
        state_ = ns;
        if (callback_) callback_(old, ns);
    }

    AppState state_ = AppState::Idle;
    StateChangeCallback callback_;
};

} // namespace sn
