# ScrollNice Slint Migration Execution Status

## Completed in this pass

1. Created a Rust + Slint migration baseline (`Cargo.toml`, `build.rs`, `src/main.rs`, `ui/main_window.slint`).
2. Implemented core Rust modules aligned with the design:
   - `ConfigStore` + config schema v2
   - `ScrollEngine`
   - `Zone` hit-testing
   - `StateMachine`
3. Added internationalization scaffolding with runtime locale loading:
   - `locales/en.json`
   - `locales/vi.json`
4. Added preset files:
   - `presets/chrome.json`
   - `presets/vscode.json`
   - `presets/edge.json`
   - `presets/firefox.json`
5. Upgraded top-level `config.json` to version 2 schema from the migration design.
6. Added unit tests for core behavior (`Zone`, `ScrollEngine`, `i18n` fallback).

## Still pending for full migration completion

1. Win32 platform bridge implementation in Rust:
   - Mouse hook
   - Input injection
   - Tray integration
   - Hotkeys
2. Zone overlay native behavior:
   - Click-through window flags
   - Layered transparency integration
3. Full preset/profile management UI and persistence actions.
4. Auto-detect scroll target integration with `WindowFromPoint` and fallback strategy.
5. End-to-end parity testing against current C++ implementation.

## Migration strategy used

- Kept C++ code intact to avoid breaking the current release path.
- Introduced Rust/Slint as a parallel implementation lane.
- Upgraded shared config schema to match the approved design document.
