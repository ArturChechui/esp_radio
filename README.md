# ESP32 Internet Radio: From Zero to One

A fully custom-built internet radio powered by an ESP32, featuring high-quality audio streaming, a custom 3D-printed enclosure, and a robust FreeRTOS-based software architecture.

## The Journey & The "Why"

This project was born out of a desire to build something truly mine, from the very first wire to the last line of code.

For a long time, my life was consumed by 24/7 work cycles and overtimes. I had the urge to create, but my head was too full of "work" to think about "engineering." After a significant life change, a failed relocation due to a technicality with residency documents, I found myself at a crossroads. Instead of letting the disappointment take over, I chose to rest, reconsider my future, and finally dive into the world I had been watching from the sidelines: **Hardware.**

This radio represents my journey of learning:

* **Soldering:** Transitioning from software to physical connections.
* **3D Modeling & Printing:** Designing an enclosure in Fusion360 and bringing it to life.
* **Acoustics:** Applying mathematical standards to physical sound chambers.
* **Embedded Programming:** Moving from high-level logic to FreeRTOS, I2S protocols, and memory management.

Today, this radio sits on my desk and plays my favorite Ukrainian stations. It's a piece of home, a source of nostalgia, and a constant reminder that even when plans fail, you can still build something beautiful.

---

## Evolution of the Build

| Phase | Development Log & Highlights |
| :--- | :--- |
| **1. Breadboard Prototyping** | <img src="https://raw.githubusercontent.com/ArturChechui/esp_radio/main/resources/photos/1_basic_breadboard.jpg" height="120"> <img src="https://raw.githubusercontent.com/ArturChechui/esp_radio/main/resources/photos/2_small_screen_breadboard.jpg" height="120"> <img src="https://raw.githubusercontent.com/ArturChechui/esp_radio/main/resources/photos/3_small_speaker_breadboard.jpg" height="120"> <img src="https://raw.githubusercontent.com/ArturChechui/esp_radio/main/resources/photos/4_semi_final_breadboard.jpg" height="120"> <img src="https://raw.githubusercontent.com/ArturChechui/esp_radio/main/resources/photos/5_all_modules_breadboard.jpg" height="120"> |
| **2. Assembly & Flashing & 3D Modeling** | <img src="https://raw.githubusercontent.com/ArturChechui/esp_radio/main/resources/photos/10_3dmodel_fusion.jpg" height="120"><br><br><img src="https://raw.githubusercontent.com/ArturChechui/esp_radio/main/resources/photos/6_assembly_box.jpg" height="120"> <img src="https://raw.githubusercontent.com/ArturChechui/esp_radio/main/resources/photos/9_internals_box.jpg" height="120"> <img src="https://raw.githubusercontent.com/ArturChechui/esp_radio/main/resources/photos/8_flashing_box.jpg" height="120"> |
| **3. Final Product** | <img src="https://raw.githubusercontent.com/ArturChechui/esp_radio/main/resources/photos/7_finished_box.png" height="120"> <img src="https://raw.githubusercontent.com/ArturChechui/esp_radio/main/resources/photos/11_finished_box.jpg" height="120"> <img src="https://raw.githubusercontent.com/ArturChechui/esp_radio/main/resources/photos/12_finished_box.jpg" height="120"> |
---

## Feature Demonstrations

Below are recorded validation tests demonstrating the core features, real-time control loops, and network resilience of the radio pipeline.

### E2E Walkthroughs

* **Full Feature Walkthrough (v1):** Showcases station cycling, rotary control, and clap-to-play activation.

[▶ Click here or the image below to watch video](https://youtu.be/mcjk8brsLNs)

[![Watch the video](https://img.youtube.com/vi/mcjk8brsLNs/hqdefault.jpg)](https://www.youtube.com/embed/mcjk8brsLNs)

* **Full Feature Walkthrough (v2):** Alternate routine verifying UI updates alongside hardware changes.

[▶ Click here or the image below to watch video](https://youtu.be/W9kkjmenZKo)

[![Watch the video](https://img.youtube.com/vi/W9kkjmenZKo/hqdefault.jpg)](https://www.youtube.com/embed/W9kkjmenZKo)

### Network Resilience & Edge-Case Buffering

* **Low-RSSI Test (v1):** Stress test under extreme signal degradation (1-bar Wi-Fi where smartphones struggle to connect). Demonstrates the system detecting the drop, entering an explicit `Buffering` state, and successfully recovering the stream.

[▶ Click here or the image below to watch video](https://youtu.be/d_K3pIjVS1A)

[![Watch the video](https://img.youtube.com/vi/d_K3pIjVS1A/hqdefault.jpg)](https://www.youtube.com/embed/d_K3pIjVS1A)

* **Low-RSSI Test (v2):** Shows the stream recovery behavior and UI state synchronization under brutal packet-loss conditions.

[▶ Click here or the image below to watch video](https://youtu.be/vzDmDu5UXB0)

[![Watch the video](https://img.youtube.com/vi/vzDmDu5UXB0/hqdefault.jpg)](https://www.youtube.com/embed/vzDmDu5UXB0)

### Hardware Peripheral Validation

* **Station Skipping & Accumulation:** Tests the physical button debounce filters and fast station navigation logic.

[▶ Click here or the image below to watch video](https://youtu.be/VR3h5mVXo3s)

[![Watch the video](https://img.youtube.com/vi/VR3h5mVXo3s/hqdefault.jpg)](https://www.youtube.com/embed/VR3h5mVXo3s)

* **Rotary Encoder Step Control:** Verification of the quarter-detent accumulator algorithm providing linear audio attenuation.

[▶ Click here or the image below to watch video](https://youtu.be/cs2J7kylFnE)

[![Watch the video](https://img.youtube.com/vi/cs2J7kylFnE/hqdefault.jpg)](https://www.youtube.com/embed/cs2J7kylFnE)

## Technical Overview

The system is designed to be modular, thread-safe, and resilient to network fluctuations.

### Acoustic & Enclosure Design

The enclosure isn't just a shell; it was engineered as a tuned acoustic chamber to maximize the performance of the drivers.

* **Custom "Golden Ratio" Geometry:** The design is based on the **Golden Ratio for Loudspeaker Enclosures** ($1 : 1.618 : 0.618$) to minimize internal standing waves and resonance.
* **Dimensional Adaptation:** To accommodate the physical depth of the high-performance speakers, the standard ratio was carefully adapted. The final outer dimensions are **145mm (W) x 90mm (H) x 55mm (D)**. With **4mm thick walls** for structural rigidity, the internal volume is optimized for the specific resonant frequency of the drivers.
* **Virtual Volume Enhancement:** The speaker chambers are filled with **Polyester Fiberfill**. This material slows down the air particles inside the box, tricking the speaker into "thinking" it is in a much larger enclosure. This provides a significantly richer sound and deeper bass response than a standard hollow 3D-printed box would allow.

### Software Stack

* **Language:** C++17
* **OS:** FreeRTOS (ESP-IDF)
* **Audio:** I2S protocol for external DAC communication.
* **Data:** JSON-based station management and system manifests.
* **Core Logic:** A custom `TaskRunner` system that manages background services (WiFi, Audio, Sensors) using a slot-based static allocation to ensure stability.

### Key Features

* **GitHub Station Sync:** By **long-pressing the Play button**, the radio automatically downloads the latest `stations.json` file directly from the main branch of the project's GitHub repository. To ensure data integrity, the system displays a status message and **blocks all physical inputs** for the 1-2 seconds it takes to perform the sync.
* **Clap-to-Play:** A fun, integrated feature that monitors the microphone for a specific sound profile, allowing the user to trigger playback with a simple clap.
* **WiFi Provisioning:** A dedicated setup mode allows the radio to host its own configuration portal, making it easy to connect to any local network without hardcoding credentials.
* **Tactile Volume Control:** Uses a rotary encoder with an accumulation algorithm (quarter-detent) to provide smooth, professional-feeling volume adjustment.
* **Dedicated Playback Buttons:** Physical buttons for **Play/Pause**, **Next**, and **Previous** stations for a "classic radio" feel that doesn't require looking at a screen.
* **OLED Interface:** A high-contrast display showing station names, real-time signal strength (RSSI), and system status icons.
* **Resilient Audio Pipeline:** A custom `RingBuffer` and `TaskRunner` architecture designed to handle network jitter and prevent audio "hiccups" during streaming.

---

## Lessons Learned

### Theory vs. Practice

The biggest takeaway from this project was the realization of the gap between theory and reality. In software, variables are usually controlled. In hardware, **everything is a variable.**

* **Signal Noise:** Real-world interference affects audio and sensor data in ways a simulator never shows.
* **Hardware Tolerance:** Components don't always behave exactly like the datasheet says.
* **Adaptability:** I learned that a good engineer doesn't just write a perfect plan; they write a plan that is flexible enough to survive the "real world."

---

## How it Works (Architecture)

1. **Network Layer:** Fetches MP3 streams via HTTP.
2. **Buffer Layer:** A thread-safe circular buffer (RingBuffer) stores raw data.
3. **Decode Layer:** An MP3 decoder task pulls from the buffer and produces PCM samples.
4. **Output Layer:** I2S pushes the samples to the external DAC/Speaker.
5. **Control Layer:** Monitors buttons and encoders to dispatch system-wide events.

---

## Hardware Components

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
- Case: Custom 3D-printed enclosure designed in Fusion360

---

## Interactive Documentation & Release Specs

The full source code architecture, API references, and system design specifications are compiled and deployed online:

* **[Live Doxygen Documentation Portal](https://arturchechui.github.io/esp_radio/doxygen/html/index.html)**
* **[System Design Specifications (Requirements.md)](https://raw.githubusercontent.com/ArturChechui/esp_radio/refs/heads/main/docs/requirements.md)**
