# Sentinel CYD tiny OS

Goal: on boot, display the `logo_png.png` on the Cheap Yellow Display (ESP32-2432S028 R v2 USB) and do nothing else.

## Build and Flash

Prereqs: VS Code + PlatformIO extension.

1. Connect the CYD via USB.
2. In PlatformIO, pick the environment `esp32dev` and Upload Filesystem Image (this uploads `data/logo_png.png` to SPIFFS).
3. Upload the firmware.

On boot, the image is drawn from SPIFFS at `/logo_png.png`.
