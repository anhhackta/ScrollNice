use anyhow::Result;

pub struct InputInjector;

impl InputInjector {
    pub fn send_scroll(_delta: i32) -> Result<()> {
        Ok(())
    }
}
