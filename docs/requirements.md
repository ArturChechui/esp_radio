# ESP32 Internet Radio – Requirements v0.1

## 0. Summary

An ESP32-S3 based internet radio device that:

- Plays online radio streams (AAC + MP3) via ESP-ADF.
- Shows a station list on a 0.96" OLED (I2C) and indicates the currently active station.
- Uses buttons for selection + play/stop and a rotary encoder for volume.
- Displays temperature & humidity in a corner of the screen (AHT20).
- Maintains a local station list and can download updates in the background (reboot to apply).

## 1. Goals & Non-Goals

### Goals

- Reliable playback of AAC/MP3 radio URLs.
- Predictable UI behavior: selection is separate from activation.
- Testable core logic (state machine, station selection, persistence rules).
- Non-blocking background station-list update.

### Non-Goals

- Metadata (ICY title/artist), album art, track history.
- Voice/clap control.
- Battery power / power management.
- Web UI / BLE UI for station management (beyond background download).
- Audius / Spotify integration (possible future versions).

## 2. Hardware Assumptions (V1)

- MCU: ESP32-S3 SuperMini
- Display: SSD1306 128x64 OLED via I2C
- Temp/Hum: AHT20 (I2C)
- Audio: 2x MAX98357 (I2S DAC/amp) + 2x 8Ohms 3W speakers
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

- The selected station shall be marked with a play icon if playing, and a stop icon if stopped.
- `Next` shall switch to the previous station and start playback.
- `Previous` shall switch to the next station and start playback.
- Selection wraps at list boundaries.

Acceptance:

- Pressing Next/Previous switches station and updates the marker within 50 ms.
- UI shows correct playback status icon for the active/selected station.

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

- `autoplay` (bool): whether device should auto-start playback on boot
- `last_station_id` (string): id of last active station
- `volume` (uint8 0..100)

Rules:

- When playback successfully transitions to `Playing`: set `autoplay=true` and store `last_station_id=<active station>`.
- When user presses `Stop` and state becomes `Stopped`: set `autoplay=false`.
- Volume persists with debounce: save 1-2 seconds after the last volume change (coalescing multiple updates).

Acceptance:

- After setting `volume=V`, rebooting, and reading settings, volume equals V.
- After a successful Play (state reaches `Playing`), `autoplay=true` and `last_station_id` equals the active station id (verified by reading NVS).
- After Stop (state becomes `Stopped`), `autoplay=false` (verified by reading NVS).
- Volume debounce: during rapid encoder rotation (≥10 changes within 2 seconds), NVS write occurs at most once per debounce window, and final stored volume equals the last value.

Notes:

- If NVS keys are missing or corrupted, fall back to defaults: `autoplay=false`, `volume=50`, `last_station_id=""`.

### FR-08 Wi-Fi configuration

- Device connects to Wi-Fi automatically on boot using build-time configured SSID/password.
- UI exposes Wi-Fi state: `Connecting`, `Connected`, `Disconnected`.
- If Wi-Fi disconnects, device retries reconnect in background.

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
- After loading, the in-RAM list remains unchanged even if `stations.json` changes on flash (updates require reboot to apply).

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

On boot, the device shall:

1. Initialize UI and show a boot status (e.g., "Starting…").
2. Load station list for the session:
   - Load `stations.json` from LittleFS (FR-09).
   - If missing/invalid, show "No stations available" and skip autoplay.
3. Load settings from NVS (FR-07): `autoplay`, `last_station_id`, `volume`.
4. Start Wi-Fi connection in background (FR-08).
5. If `autoplay=true`, attempt to start playback of `last_station_id` after:
   - station list is loaded, and
   - Wi-Fi is connected (or once it becomes connected).

Behavior:

- If `autoplay=true` but `last_station_id` is not found in the loaded station list, the device shall show "Last station missing" and remain stopped.
- If station list is unavailable, autoplay is ignored for this boot (device remains stopped).
- Boot shall not block UI responsiveness; playback start runs asynchronously.

Acceptance:

- Device reaches a usable UI state (station list visible) without requiring network connectivity.
- If `autoplay=false`, device stays stopped and does not start playback automatically.
- If `autoplay=true` and last station exists, device eventually starts playback after Wi-Fi connects (no manual input required).
- If `autoplay=true` and last station is missing, device shows "Last station missing" and remains stopped.

Notes:

- "Eventually" for autoplay means: once Wi-Fi is connected; exact timing is network-dependent.

### FR-11 Background station list update (GitHub)

- Device shall support downloading an updated station list from a remote URL and storing it as `stations.json` in LittleFS.
- The in-RAM station list for the current session shall not change; updates are applied on next reboot.

Start condition:

- Updater starts only when:
  - Wi-Fi is `Connected`, and
  - playback is stable (player state is `Playing` continuously for 10 seconds).

Update mechanism:

- Updater shall first fetch `stations.manifest.json` (small file) containing:
  - `{ "version": <int>, "sha256": "<hex>", "size": <int> }`
- If manifest indicates change vs the last applied manifest, download `stations.json` to a temporary file.
- Validate the downloaded JSON (same rules as FR-09).
- If valid, replace local `stations.json` atomically (temp -> rename).
- Notify UI: "Station list updated. Reboot to apply."

Retry/backoff:

- On network/HTTP failure, retry with exponential backoff:
  - 60s -> 2m -> 4m -> 8m -> 16m -> 30m (cap).

Invalid remote list

- If downloaded `stations.json` is invalid, keep existing local file unchanged and continue retries with the defined backoff.
- If only invalid results occur during all the retries, then:
  - show one UI message: "Remote list invalid (paused)"
  - stop updater until reboot.

No update available

- If no updates are found during all the retries, then:
  - show one UI message: "List up-to-date (paused)"
  - stop updater until reboot.

Update success

- If an update is successfully downloaded + validated + stored, then:
  - show one UI message: "Station list updated. Reboot to apply."
  - stop updater until reboot.

Acceptance:

- Updater starts only when Wi-Fi is `Connected` and playback is stable (`Playing` for 10s).
- If manifest is unchanged, `stations.json` is not downloaded.
- If manifest changed and downloaded JSON is valid, local `stations.json` is replaced atomically and UI shows once: "Station list updated. Reboot to apply."
- If updater runs all the attemps without a successful update:
  - invalid JSON only -> show once: "Remote list invalid (paused)"
  - no updates found -> show once: "List up-to-date (paused)"
- Retry/backoff follows 60s -> 2m -> 4m -> 8m -> 16m -> 30m (cap).
- Current session station list does not change until reboot.

Notes:

- Manifest comparison can be by `version` (primary) and/or `sha256` (integrity).
- HTTP implementation uses ESP-IDF `esp_http_client`.
- URLs are hosted on GitHub (raw) or any static host.

### FR-12 Logging (local)

- Device shall output logs to serial console (`idf.py monitor`) for debugging.
- Log level shall be configurable at build-time (default: INFO).
- Logs shall include key events: boot, Wi-Fi connect/disconnect, play/stop, and station list update result.

Acceptance:

- Serial logs contain boot message, Wi-Fi state changes, play/stop actions, and updater final outcome.
- Default build logs at INFO level; a debug build can enable more verbose logs.
- On failures (invalid station list, playback error), an ERROR log entry is produced.

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
- Updater decision logic (manifest comparison, backoff schedule, "pause all the attempts" rules)
- JSON validation

Target/integration tests:

- AHT20 read sanity
- LittleFS read/write
- Basic playback smoke on ESP32-S3

## 6. V2/V3 candidates

- V2: metadata, better UI layouts, battery, clap/voice control.
- V3: Audius station/track browsing (if desired), Wi-Fi provisioning via BLE or captive portal.
