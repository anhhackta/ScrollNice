use super::config::ZoneConfig;

#[derive(Debug, Clone)]
pub struct Zone {
    pub x: i32,
    pub y: i32,
    pub width: i32,
    pub height: i32,
}

impl Zone {
    pub fn from_config(cfg: &ZoneConfig) -> Self {
        Self {
            x: cfg.x,
            y: cfg.y,
            width: cfg.width,
            height: cfg.height,
        }
    }

    pub fn contains(&self, px: i32, py: i32) -> bool {
        px >= self.x && px <= self.x + self.width && py >= self.y && py <= self.y + self.height
    }
}

#[cfg(test)]
mod tests {
    use super::Zone;
    use crate::core::config::ZoneConfig;

    #[test]
    fn zone_hit_test_works() {
        let z = Zone::from_config(&ZoneConfig {
            x: 10,
            y: 20,
            width: 100,
            height: 200,
            opacity: 0.3,
            locked: false,
        });
        assert!(z.contains(50, 50));
        assert!(!z.contains(500, 500));
    }
}
