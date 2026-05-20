# ESP32 Internet Radio – Requirements

## 0. Summary

An ESP32-S3 based internet radio device that:

- Plays online radio streams (AAC + MP3) via ESP-ADF.
- Shows a station list on a 0.96" OLED (I2C) and indicates the currently active station.
- Uses buttons for selection + play/stop and a rotary encoder for volume.
- Displays temperature & humidity in a corner of the screen (AHT20).
- Reads ambient light and applies day/night input mode behavior.
- Monitors battery level and shows battery status in the UI.
- Maintains a local station list and supports on-demand sync from GitHub.

## 1. Goals & Non-Goals

### Goals

- Reliable playback of AAC/MP3 radio URLs.
- Predictable UI behavior: selection is separate from activation.
- Testable core logic (state machine, station selection, persistence rules).
- Safe station-list synchronization with manifest check and file swap.
- Integrated light, clap, and battery sensing modules.

### Non-Goals

- Metadata (ICY title/artist), album art, track history.
- Web UI / BLE UI for station management (beyond on-device sync).
- Audius / Spotify integration.

## 2. Hardware Configuration

- MCU: ESP32-S3 SuperMini
- Display: 1.3" SH1106 128x64 OLED via I2C
- Temp/Hum: AHT20 (I2C)
- Light sensor: BH1750 (I2C)
- Sound sensor: GY-MAX4466 (ADC)
- Audio modules: 2x MAX98357 (I2S DAC/amp)
- Speakers: 2x AIYIMA 2" 4 Ohms 3W
- Battery: 3x 18650 Li-ion, 3000 mAh each
- Inputs:
  - Buttons: Next, Previous, Play/Stop
  - Rotary encoder EC11 for Volume

## 3. Functional Requirements (FR)

### FR-01 Station list display

- The device shall display a list of stations.
- Next to 6 stations shall be visible at once.
- Station names shall be truncated if they exceed display width.

Acceptance:

- Device displays stations on screen.

### FR-02 Play

- `Play/Stop` button shall start playback of the currently selected station if stopped.

Acceptance:

- Given a reachable URL and Wi-Fi connected, audio starts within a reasonable time (informational target: P50 < 3s).

### FR-03 Stop

- Playback stops when `Play/Stop` is pressed during active playback.

Acceptance:

- Stop transitions to `Stopped` state and audio output ceases.

### FR-04 Active station indicator & navigation

- The selected station shall be marked with a playback status icon (Play, Stop, Buffering).
- `Next` shall increment to the next logical station in the list.
- `Previous` shall decrement to the previous logical station in the list.
- Selection wraps at list boundaries.
- Multiple rapid presses on `Next` or `Previous` shall quickly browse through station names on the UI without initializing the network stream or starting playback on each hop.

Acceptance:

- Pressing Next/Previous updates the selected station index and renders the text change within 50 ms.
- Rapidly pounding the navigation buttons updates the screen but does not trigger audio decoding errors or network thread crashes.

### FR-05 Volume control (rotary encoder)

- Rotating encoder changes volume.
- Volume range: 0..100 (step: 2 by default).
- Volume changes apply immediately.

Acceptance:

- Rotating encoder updates volume and shows it on UI within 100 ms.

### FR-06 Temp/Humidity display

- Read AHT20 periodically (e.g., every 10 seconds).
- Display current temperature and humidity in a small UI corner.

Acceptance:

- Temp/Hum values update periodically and do not disrupt playback.

### FR-07 Persistence (NVS)

Persist in NVS:

- `station_idx` (`uint32_t`): The active zero-based index of the currently selected radio station.
- `volume` (`uint32_t`, range `0..100`): The current system audio output level.
- `wifi_ssid` (`string`): The network service set identifier captured via provisioning.
- `wifi_password` (`string`): The network security key captured via provisioning.
- `micfeature` (`uint32_t`, `0` or `1`): The master toggle state for the acoustic clap-to-play trigger feature (`0` = disabled, `1` = enabled).

Acceptance:

- Given an adjusted volume value $V$, after a hardware reset or brownout cycle, the system initializes and restores the audio output level exactly to $V$.
- Given a station switch to index $I$, after a complete system power cycle, the UI and audio components boot directly into station index $I$.
- Given a state toggle where `toggleClapFeature()` sets claps to active, after a device reboot, the `micfeature` value reads as `1` and the `UiService` correctly restores the 1-pixel bitwise top ledger line over the volume block.
- Given a successful network configuration cycle through the portal, the captured SSID and password must be successfully written to `wifi_ssid` and `wifi_password` respectively.

### FR-08 Wi-Fi configuration

- Device connects to Wi-Fi automatically on boot using stored credentials.
- Provisioning Portal: If credentials are missing or connection fails, the device shall host a local captive provisioning network portal.
- Input Constraints: The provisioning web UI portal shall explicitly denote that only Latin-based ASCII characters are supported for SSID and Passwords to prevent downstream configuration parsing corruption.

Acceptance:

- With valid credentials, device reaches `Connected` within 30s after boot.
- With invalid credentials/unavailable AP, device reaches `Disconnected` within 30s and UI remains usable.
- After a disconnect event, device retries and can return to `Connected` without reboot.

### FR-09 Station list storage (LittleFS)

- Device shall store the station list as `stations.json` in LittleFS.
- On boot, device shall load `stations.json` into RAM and use it for the whole session.
- Station list schema is JSON array of objects `{id,name,url}`.

Constraints:

- cap: 10 stations.

Acceptance:

- If `stations.json` exists and is valid, device loads it and displays stations in the UI.
- If `stations.json` is missing or invalid, device displays "No stations available" and playback cannot be started.
- If `stations.json` becomes valid on the next boot, station list is shown and playback becomes available.
- After loading, the in-RAM list remains unchanged until an explicit sync is completed (FR-11).

JSON schema:

```json
[
  {"id":"radio1_aac_h","name":"Radio 1 (AAC High)","url":"https://...aac"},
  {"id":"some_mp3","name":"Some MP3","url":"https://...mp3"}
]
```

Notes:

- Validation rules: non-empty `id/name/url`, url starts with `http://` or `https://`.
- IDs must be unique within the file.

### FR-10 Boot behavior

On boot, the device shall execute the following sequence:

1. Initialize UI: Show "Booting..." screen immediately.
2. Load NVS: Read `volume`, `station_idx`, and `micfeature` settings.
3. Load FS: Load `stations.json` from LittleFS into RAM.
4. Network Check & Route: Read `wifi_ssid` and `wifi_password`.
4.1 If valid/connected: Route to Main Screen, display station name from `station_idx`.
4.2 If missing/failed: Route to Provisioning Screen and display a small instruction and a QR code to the portal.

Acceptance:

- Happy Path: With valid credentials stored, the radio boots directly to the Main station screen without showing the provisioning portal.
- Fallback Path: If credentials are missing or connection fails, the radio automatically opens the captive portal and renders the QR code with an instruction.
- Missing Playlist: If `stations.json` is missing or corrupt, the UI displays a "No stations available" error and blocks playback actions.

### FR-11 Station list synchronization (GitHub, on-demand)

- A long press on `Play/Stop` (3 seconds) shall start station synchronization.
- While sync is active, UI shall switch to a dedicated sync screen and all user inputs shall be ignored.
- Sync flow:
  - Download remote `manifest.json`.
  - Compare local and remote manifest `version`.
  - If versions differ, download remote `stations.json`.
  - Validate downloaded station JSON using the same schema/rules as FR-09.
  - Stage to temporary files and replace live files.
  - Reload station repository in the running session (no reboot required).
- If manifest is unchanged, sync exits without replacing files.

Failure handling:

- On Wi-Fi/HTTP failure, parse error, validation failure, or swap failure:
  - keep current files unchanged
  - exit sync
  - return UI to main screen

Acceptance:

- Long press (`>=3s`) on `Play/Stop` triggers sync mode.
- During sync, button/encoder input does not start/stop playback and does not change station/volume.
- If manifest version is unchanged, `stations.json` is not downloaded.
- If manifest version changed and downloaded JSON is valid, `stations.json` and `manifest.json` are swapped and station list reloads immediately.
- On any sync error, app returns to main screen and previously valid files remain active.

Notes:

- Manifest comparison uses `version` as the primary key.
- URLs are hosted on GitHub raw content.

### FR-12 Logging (local)

- Device shall output logs to serial console (`idf.py monitor`) for debugging.
- Log level shall be configurable at build-time (default: INFO).
- Logs shall include key events: boot, Wi-Fi connect/disconnect, play/stop, and station list update result.

Acceptance:

- Serial logs contain boot message, Wi-Fi state changes, play/stop actions, and updater final outcome.
- Default build logs at INFO level; a debug build can enable more verbose logs.
- On failures (invalid station list, playback error), an ERROR log entry is produced.

### FR-13 Light, clap and power modules

- Device shall read ambient light periodically and expose a `LightLevelUpdate` event.
- Device shall read battery voltage and estimated percentage periodically and expose a `BatteryLevelUpdate` event.
- UI shall show battery state icon (low/mid/full).
- Device shall run clap detection logic utilizing a dedicated FreeRTOS processing task monitoring microphone input.
- A Long Press on the `Next` button shall invoke `toggleClapFeature()`, flipping the master setting (`mClapFeatureEnabled`).
- Execution calls to activate detection (`startClapDetection(true)`) shall be conditionally checked against the master user setting (`mClapFeatureEnabled`). If the feature is toggled off, the execution request is ignored.
- When the master clap feature is active, an event (`ClapFeatureStateChangedEvent`) shall notify the UI service, drawing a minimalist 1-pixel-thick bitwise notification overlay at the top edge of the volume status block icon.

Acceptance:

- Light sensor values are produced periodically and can drive day/night input behavior.
- Battery voltage/percent values are produced periodically and battery icon updates accordingly.
- Long-pressing `Next` successfully flips the master state, saves the new value to NVS, and triggers a UI redraw.
- The clap detection task only runs when playback is stopped and the master feature state is enabled. If the feature is disabled, the microphone task remains completely inactive.

## 4. Non-Functional Requirements (NFR)

- NFR-01 Reliability: no crashes during 4-hour playback soak.
- NFR-02 Responsiveness: button actions should feel instant (UI update < ~100 ms).
- NFR-03 Resource: station list stored in RAM; max 10 items.
- NFR-04 Observability: log levels configurable at compile-time (INFO default).

## 5. Testing Strategy

Host unit tests:

- Station selection rules
- Player state machine transitions
- Persistence rules (autoplay + last_station_id + volume debounce)
- Station sync decision logic (manifest comparison, validation, swap, reload)
- JSON validation
- Light, battery, and clap processing logic

Target/integration tests:

- Temp, Light, Battery read sanity
- Clap module trigger
- LittleFS read/write
- Basic playback smoke on ESP32-S3
