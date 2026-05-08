#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ThemeMode {
    Light,
    Dark,
}

impl ThemeMode {
    pub fn from_config(value: &str) -> Self {
        match value {
            "light" => ThemeMode::Light,
            _ => ThemeMode::Dark,
        }
    }

    pub fn as_str(self) -> &'static str {
        match self {
            ThemeMode::Light => "light",
            ThemeMode::Dark => "dark",
        }
    }
}
