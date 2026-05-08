#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ScrollState {
    Idle,
    HoveringUp,
    HoveringDown,
    HoldingUp,
    HoldingDown,
}

#[derive(Debug)]
pub struct StateMachine {
    state: ScrollState,
}

impl StateMachine {
    pub fn new() -> Self {
        Self {
            state: ScrollState::Idle,
        }
    }

    pub fn state(&self) -> ScrollState {
        self.state
    }

    pub fn set_state(&mut self, next: ScrollState) {
        self.state = next;
    }
}
