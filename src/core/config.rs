use anyhow::{Context, Result};
use serde::{Deserialize, Serialize};
use std::fs;
use std::path::{Path, PathBuf};

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum ScrollMode {
    ClickHold,
    SplitHold,
    HoverAuto,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum WheelBlockMode {
    Off,
    Global,
    OutsideZoneOnly,
    InsideZoneOnly,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct HotkeysConfig {
    pub toggle_enabled: String,
    pub toggle_edit: String,
    pub toggle_wheel: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ScrollConfig {
    pub mode: ScrollMode,
    pub scroll_amount: i32,
    pub continuous_speed: i32,
    pub continuous_accel: f32,
    pub hover_speed: i32,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ZoneConfig {
    pub x: i32,
    pub y: i32,
    pub width: i32,
    pub height: i32,
    pub opacity: f32,
    #[serde(default)]
    pub locked: bool,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SoundConfig {
    pub enabled: bool,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Profile {
    pub id: String,
    pub name: String,
    pub config: AppConfig,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct AppConfig {
    pub version: u32,
    pub enabled: bool,
    pub language: String,
    pub theme: String,
    pub current_profile: String,
    pub hotkeys: HotkeysConfig,
    pub scroll: ScrollConfig,
    pub zone: ZoneConfig,
    pub wheel_block: WheelBlockMode,
    pub wheel_block_bypass_modifier: String,
    pub start_with_windows: bool,
    pub sound: SoundConfig,
    #[serde(default)]
    pub profiles: Vec<Profile>,
}

impl Default for AppConfig {
    fn default() -> Self {
        Self {
            version: 2,
            enabled: true,
            language: "en".to_string(),
            theme: "dark".to_string(),
            current_profile: "default".to_string(),
            hotkeys: HotkeysConfig {
                toggle_enabled: "Ctrl+Alt+S".to_string(),
                toggle_edit: "Ctrl+Alt+E".to_string(),
                toggle_wheel: "Ctrl+Alt+W".to_string(),
            },
            scroll: ScrollConfig {
                mode: ScrollMode::HoverAuto,
                scroll_amount: 3,
                continuous_speed: 5,
                continuous_accel: 1.2,
                hover_speed: 3,
            },
            zone: ZoneConfig {
                x: 0,
                y: 100,
                width: 60,
                height: 400,
                opacity: 0.3,
                locked: false,
            },
            wheel_block: WheelBlockMode::Off,
            wheel_block_bypass_modifier: "Alt".to_string(),
            start_with_windows: false,
            sound: SoundConfig { enabled: false },
            profiles: Vec::new(),
        }
    }
}

#[derive(Debug, Clone)]
pub struct ConfigStore {
    path: PathBuf,
    config: AppConfig,
}

impl ConfigStore {
    pub fn load_or_default(path: impl AsRef<Path>) -> Result<Self> {
        let path = path.as_ref().to_path_buf();
        let config = match fs::read_to_string(&path) {
            Ok(raw) => match serde_json::from_str::<AppConfig>(&raw) {
                Ok(cfg) => cfg,
                Err(_) => AppConfig::default(),
            },
            Err(_) => AppConfig::default(),
        };

        Ok(Self { path, config })
    }

    pub fn get(&self) -> &AppConfig {
        &self.config
    }

    pub fn update(&mut self, config: AppConfig) {
        self.config = config;
    }

    pub fn save(&self) -> Result<()> {
        let json = serde_json::to_string_pretty(&self.config).context("serialize config")?;
        fs::write(&self.path, json).with_context(|| format!("write config {}", self.path.display()))
    }
}
