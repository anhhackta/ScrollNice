use crate::core::config::{AppConfig, ConfigStore, WheelBlockMode};
use crate::i18n::I18nStore;
use crate::themes::ThemeMode;
use anyhow::Result;
use std::sync::{Arc, Mutex};

slint::include_modules!();

fn wheel_mode_label(mode: &WheelBlockMode) -> &'static str {
    match mode {
        WheelBlockMode::Off => "Off",
        WheelBlockMode::Global => "Global",
        WheelBlockMode::OutsideZoneOnly => "Outside Zone",
        WheelBlockMode::InsideZoneOnly => "Inside Zone",
    }
}

fn sync_ui_from_config(ui: &AppWindow, cfg: &AppConfig, i18n: &I18nStore) {
    ui.set_app_title(i18n.tr("app_name").into());
    ui.set_language(cfg.language.clone().into());
    ui.set_theme_mode(ThemeMode::from_config(&cfg.theme).as_str().into());
    ui.set_scroll_mode(format!("{:?}", cfg.scroll.mode).into());
    ui.set_wheel_block_mode(wheel_mode_label(&cfg.wheel_block).into());
    ui.set_profile_name(cfg.current_profile.clone().into());
}

pub async fn run_ui(store: ConfigStore) -> Result<()> {
    let store = Arc::new(Mutex::new(store));
    let cfg = store.lock().expect("config lock poisoned").get().clone();
    let i18n = I18nStore::load("locales", &cfg.language)?;

    let ui = AppWindow::new()?;
    sync_ui_from_config(&ui, &cfg, &i18n);

    let store_for_save = Arc::clone(&store);
    ui.on_request_save(move || {
        if let Ok(guard) = store_for_save.lock() {
            let _ = guard.save();
        }
    });

    let ui_weak = ui.as_weak();
    ui.on_request_close(move || {
        if let Some(ui) = ui_weak.upgrade() {
            let _ = ui.hide();
        }
    });

    ui.run()?;
    Ok(())
}
