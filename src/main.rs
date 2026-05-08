use anyhow::Result;
use scrollnice_rs::core::config::ConfigStore;
use scrollnice_rs::ui::bridge::run_ui;
use tracing_subscriber::EnvFilter;

fn init_tracing() {
    let filter = EnvFilter::try_from_default_env().unwrap_or_else(|_| EnvFilter::new("info"));
    tracing_subscriber::fmt().with_env_filter(filter).init();
}

#[tokio::main]
async fn main() -> Result<()> {
    init_tracing();

    let config_store = ConfigStore::load_or_default("config.json")?;
    run_ui(config_store).await
}
