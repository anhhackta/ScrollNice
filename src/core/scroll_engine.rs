#[derive(Debug, Clone)]
pub struct ScrollEngine {
    base_amount: i32,
    continuous_speed: i32,
    accel: f32,
}

impl ScrollEngine {
    pub fn new(base_amount: i32, continuous_speed: i32, accel: f32) -> Self {
        Self {
            base_amount,
            continuous_speed,
            accel,
        }
    }

    pub fn click_delta(&self, direction: i32) -> i32 {
        direction * self.base_amount
    }

    pub fn hold_delta(&self, direction: i32, elapsed_seconds: f32) -> i32 {
        let speed = self.continuous_speed as f32 * (1.0 + self.accel * elapsed_seconds.max(0.0));
        (direction as f32 * speed).round() as i32
    }
}

#[cfg(test)]
mod tests {
    use super::ScrollEngine;

    #[test]
    fn click_delta_uses_direction() {
        let engine = ScrollEngine::new(3, 5, 1.2);
        assert_eq!(engine.click_delta(1), 3);
        assert_eq!(engine.click_delta(-1), -3);
    }

    #[test]
    fn hold_delta_scales_with_elapsed_time() {
        let engine = ScrollEngine::new(3, 5, 1.0);
        let early = engine.hold_delta(1, 0.2);
        let later = engine.hold_delta(1, 1.0);
        assert!(later > early);
    }
}
