use anyhow::Result;

pub struct MouseHook;

impl MouseHook {
    pub fn install() -> Result<Self> {
        Ok(Self)
    }
}
