use anyhow::Result;

pub struct Hotkeys;

impl Hotkeys {
    pub fn register_defaults() -> Result<Self> {
        Ok(Self)
    }
}
