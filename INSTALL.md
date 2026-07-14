# Installation Guide

This guide covers setting up a development environment on WSL (Ubuntu) to build, test, and flash the ESP32 firmware.

## 1. System Dependencies & WSL Setup

Ensure you are running an updated Ubuntu environment inside WSL, then install the required core build tools:

```bash
# Update system packages
sudo apt update && sudo apt upgrade -y

# Install core build dependencies
sudo apt install git wget flex bison gperf python3 python3-pip python3-venv \
                 cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0 gcovr -y

```

## 2. ESP-IDF Framework Installation (v6.0)

Clone the Esperessif IoT Development Framework and install the compilation toolchain tailored for the target hardware architecture:

```bash
# Create and navigate to the framework directory
mkdir -p ~/esp
cd ~/esp

# Clone the repository recursively
git clone -b release/v6.0 --recursive [https://github.com/espressif/esp-idf.git](https://github.com/espressif/esp-idf.git)
cd ~/esp/esp-idf

# Install compilation tools optimized for the ESP32-S3 chip
./install.sh esp32s3

# Add an environment shortcut alias to your bash configuration
echo "alias get_idf='. \$HOME/esp/esp-idf/export.sh'" >> ~/.bashrc

```

> 💡 **Usage:** Open a new terminal or run `source ~/.bashrc`. You can now load the ESP-IDF toolchain in any session by typing `get_idf`.

## 3. Project Dependencies & Unit Testing Tools

Navigate to your workspace directory to configure the external library manifests and install the host testing frameworks:

```bash
# Register component manifest dependencies
idf.py add-dependency "espressif/cjson"

# Install Google Test and Google Mock libraries for host-driven unit tests
sudo apt install libgtest-dev libgmock-dev

```

## 4. USB Device Passthrough (WSL)

To flash the device and monitor serial output directly from your WSL Ubuntu terminal, you must share the USB port from Windows using `usbipd`.

1. **On Windows** (Open PowerShell as Administrator):
```powershell
# Install the usbipd tool
winget install dorssel.usbipd-win

# List connected USB devices to find your ESP32 Bus ID (e.g., 4-1)
usbipd list

# Bind the device (only needed once)
usbipd bind --busid <BUSID>

# Attach the device to your running WSL instance
usbipd attach --wsl --busid <BUSID>

```


2. **On WSL (Ubuntu)**:
Grant your user account permission to read and write to serial devices without requiring `sudo`:
```bash
sudo usermod -aG dialout $USER

```

*(Note: You will need to restart your WSL terminal session for the group changes to take effect).*

## 5. Building, Flashing & Monitoring Logs

To build, flash, and debug the project, you can either use the raw ESP-IDF command-line interfaces or the convenient `./tools/build.sh` script included in the workspace root.

### Option A: Using the Build Script (Recommended)

Make sure the script has execution permissions:

```bash
chmod +x ./tools/build.sh

```

**Usage Commands:**

```bash
./build.sh        # Executes 'idf.py build'
./build.sh -u     # Compiles unit tests (host)
./build.sh -r     # Runs unit tests (compiles them first if needed)
./build.sh -f     # Flashes the target device
./build.sh -m     # Opens the serial monitor
./build.sh -F     # Performs a sequential flash + monitor run
./build.sh -c     # Executes 'idf.py fullclean'
./build.sh -h     # Displays helper usage details

```

### Option B: Using Native `idf.py`

If you prefer running commands manually, initialize your shell session and invoke the utilities:

```bash
# Initialize ESP-IDF environment variables
get_idf

# Build the project, flash it, and open the serial monitor in one go
idf.py build flash monitor

```

### Useful Monitor Keyboard Shortcuts

* **`Ctrl + ]`**: Exit the monitor and release the serial port.
* **`Ctrl + T` then `Ctrl + H**`: View the built-in ESP-IDF monitor help menu for advanced logging shortcuts.
