# Changelog

All notable changes to the ESP32 Internet Radio project will be documented in this file. This project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html) (`MAJOR.MINOR.PATCH`).

---

## [1.0.0] - 2026-05-20

### Added

* **Features:** Initial implementation of station display, audio playback (play, stop, skip), and Wi-Fi provisioning portal (`03756e4`, `48f2040`).
* **UI Controls:** Added rotary encoder volume adjustments with partial display updates (`62ff459`, `ffdba7e`).
* **Sensors:** Added real-time temperature, humidity, battery level, light, and acoustic clap sensing modules (`1d083b5`, `46d2689`).
* **Documentation:** Added comprehensive hardware wiring maps, component layout overviews, and system class/adapter/command diagrams (`ced8f07`, `e4f9ef6`, `d16504b`, `2ad77b2`).
* **CI/CD:** Added automated build checks via `ci.yml` alongside initial project boilerplate (`5e75130`, `5c727b9`, `052dc31`).

### Changed

* **Station Navigation:** Optimized station switching logic to support fast scrolling without blocking the audio streaming pipeline (`f63e0e0`).
* **Clap Detection:** Gated the microphone processing loop behind a master feature toggle mapped to a long-press on the Next button (`f63e0e0`).
* **Captive Portal:** Refactored the provisioning layout to include explicit Latin character constraints and browser compatibility tips (`f63e0e0`).
* **Specifications:** Refined and updated `Requirements.md` to align style, phrasing, and storage keys with the physical codebase (`ef2fcec`, `5048ddb`, `f0faed3`).

### Fixed

* **Diagrams & Mapping:** Corrected errors in the original system class structures and physical GPIO wiring layouts (`561d167`, `2ad77b2`).
