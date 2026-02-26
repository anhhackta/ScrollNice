#pragma once
#include <functional>

namespace sn {

enum class AppState {
    Disabled,
    Idle,
    Hover,
    Active,
    Edit
};

class StateMachine {
public:
    using StateChangeCallback = std::function<void(AppState oldState, AppState newState)>;

    AppState State() const { return state_; }

    void SetCallback(StateChangeCallback cb) { callback_ = cb; }

    void SetEnabled(bool on) {
        if (on && state_ == AppState::Disabled) {
            TransitionTo(AppState::Idle);
        } else if (!on && state_ != AppState::Disabled) {
            TransitionTo(AppState::Disabled);
        }
    }

    void ToggleEnabled() {
        SetEnabled(state_ == AppState::Disabled);
    }

    void OnEnterZone() {
        if (state_ == AppState::Idle) {
            TransitionTo(AppState::Hover);
        }
    }

    void OnLeaveZone() {
        if (state_ == AppState::Hover || state_ == AppState::Active) {
            TransitionTo(AppState::Idle);
        }
    }

    void OnMovementInZone() {
        if (state_ == AppState::Hover) {
            TransitionTo(AppState::Active);
        }
        // If already Active, stay Active
    }

    void ToggleEdit() {
        if (state_ == AppState::Edit) {
            TransitionTo(AppState::Idle);
        } else if (state_ != AppState::Disabled) {
            TransitionTo(AppState::Edit);
        }
    }

    bool IsScrolling() const { return state_ == AppState::Active; }
    bool IsEnabled()   const { return state_ != AppState::Disabled; }
    bool IsEditing()   const { return state_ == AppState::Edit; }

private:
    void TransitionTo(AppState newState) {
        if (state_ == newState) return;
        AppState old = state_;
        state_ = newState;
        if (callback_) callback_(old, newState);
    }

    AppState state_ = AppState::Idle;
    StateChangeCallback callback_;
};

} // namespace sn
