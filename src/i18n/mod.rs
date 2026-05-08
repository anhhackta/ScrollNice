use anyhow::{Context, Result};
use serde_json::Value;
use std::collections::HashMap;
use std::fs;
use std::path::Path;

#[derive(Debug, Clone)]
pub struct I18nStore {
    pub lang: String,
    messages: HashMap<String, String>,
}

impl I18nStore {
    pub fn load(base_dir: impl AsRef<Path>, lang: &str) -> Result<Self> {
        let path = base_dir.as_ref().join(format!("{lang}.json"));
        let fallback = base_dir.as_ref().join("en.json");

        let content = fs::read_to_string(&path)
            .or_else(|_| fs::read_to_string(&fallback))
            .with_context(|| format!("unable to load locale file {}", path.display()))?;

        let parsed: Value = serde_json::from_str(&content).context("invalid locale json")?;
        let mut messages = HashMap::new();
        flatten("", &parsed, &mut messages);

        Ok(Self {
            lang: lang.to_string(),
            messages,
        })
    }

    pub fn tr(&self, key: &str) -> String {
        self.messages
            .get(key)
            .cloned()
            .unwrap_or_else(|| key.to_string())
    }
}

fn flatten(prefix: &str, value: &Value, out: &mut HashMap<String, String>) {
    match value {
        Value::Object(map) => {
            for (k, v) in map {
                let next = if prefix.is_empty() {
                    k.to_string()
                } else {
                    format!("{prefix}.{k}")
                };
                flatten(&next, v, out);
            }
        }
        Value::String(s) => {
            out.insert(prefix.to_string(), s.to_string());
        }
        _ => {}
    }
}

#[cfg(test)]
mod tests {
    use super::I18nStore;

    #[test]
    fn fallback_works_when_key_missing() {
        let store = I18nStore::load("locales", "en").expect("load en locale");
        assert_eq!(store.tr("settings.general"), "General");
        assert_eq!(store.tr("missing.key"), "missing.key");
    }
}
