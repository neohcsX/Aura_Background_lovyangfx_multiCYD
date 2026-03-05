# Aura (PlatformIO + LovyanGFX Fork)

Aura is a weather display / widget for ESP32-based **Cheap Yellow Display (CYD)** boards with a 2.8" SPI TFT.

This repository contains a **PlatformIO port** of Aura with additional features such as touch gestures, image slideshow from SD card, and a startup animation.

---

## Original Project

This project is based on the original **Aura** project by Surrey Homeware:

https://github.com/Surrey-Homeware/Aura

The original project was designed for the Arduino IDE and includes case design and assembly instructions.

Case / assembly instructions (original project):

https://makerworld.com/en/models/1382304-aura-smart-weather-forecast-display

---

## Key Features (This Fork)

### Weather UI

* Weather widget UI for CYD boards
* Uses **LVGL** for UI rendering

### Touch & Gestures

Touch support is implemented via **LovyanGFX**.

Features include:

* integrated touch handling
* touchscreen calibration
* **Swipe down (top → bottom)** opens the configuration screen
* swipe gestures to change images

### SD Card Image Support

The device can display images loaded from an SD card.

Features:

* supports **BMP images**
* image resolution **120 × 160 pixels**
* images are loaded dynamically from the SD card
* images rotate periodically
* images can also be switched manually using swipe gestures

### Startup Animation

A short animation is displayed during device startup.

---

## Supported Hardware

This firmware targets **ESP32 Cheap Yellow Display (CYD)** style boards with a 2.8" TFT display.

Two display controller variants are supported:

* **ILI9341**
* **ST7789**

The correct variant can be selected in **PlatformIO** via the available environments.

---

## Building with PlatformIO

1. Install the **PlatformIO** extension (recommended: VS Code)
2. Clone this repository
3. Open the project folder in PlatformIO
4. Build & Upload firmware

---

## SD Card Images

Images can be loaded from the SD card and displayed on the screen.

Requirements:

* Format: **BMP**
* Resolution: **120 × 160 pixels**
* RGB565 uncompressed or 24 Bit

Images will automatically rotate and can also be changed manually using swipe gestures.

## Converting Images for SD Card

Example using ImageMagick:

convert input.jpg \
  -auto-orient \
  -resize 120x160^ -gravity center -extent 120x160 \
  -background black -alpha remove \
  -type TrueColor \
  -compress none \
  -define bmp:subtype=RGB565 \
  output.bmp

Converted images should be copied to the SD card in the folder:

/backgrounds

Example SD card layout:

SD Card
└── backgrounds
    ├── bg01.bmp
    ├── bg02.bmp
    └── bg03.bmp

---

## Touch Calibration

Touch calibration is handled by **LovyanGFX** and can be triggered through the firmware interface.

---

## Credits

### Original Project

This fork is based on the **Aura Weather Display** project created by Surrey Homeware.

Original repository:
https://github.com/Surrey-Homeware/Aura

The original project was designed for the Arduino IDE and served as the foundation for this PlatformIO-based version.

### Weather Icons

Weather icons from:

https://github.com/mrdarrengriffin/google-weather-icons/tree/main/v2

Note:
Weather icons are not necessarily covered by the GPL license. Please refer to the icon repository for their license details.

### Libraries & Resources

This project uses the following libraries:

- LVGL – graphics library for embedded UIs  
  https://lvgl.io/

- LovyanGFX – display and touch driver abstraction  
  https://github.com/lovyan03/LovyanGFX

- WiFiManager – WiFi configuration portal  
  https://github.com/tzapu/WiFiManager

- ArduinoJson – JSON parsing library  
  https://arduinojson.org/

- ArduinoHttpClient – HTTP client library  
  https://github.com/arduino-libraries/ArduinoHttpClient

All libraries are automatically installed by PlatformIO.

---

## License

The source code in this repository is licensed under the **GNU General Public License v3.0 (GPL-3.0)**.

See the `LICENSE` file for the full license text.

Third-party assets (such as icons) may use different licenses. Refer to the respective sources for details.
