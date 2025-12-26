# CryptoBar Libraries (`lib/`)

This directory is reserved for project-specific private libraries.

**Current status:** CryptoBar does not use any custom libraries in this directory. All dependencies are managed through PlatformIO's library manager.

---

## External Libraries Used by CryptoBar

CryptoBar uses the following external libraries (configured in `platformio.ini`):

| Library | Purpose | Version |
|---------|---------|---------|
| **GxEPD2** | E-paper display driver for 2.9" display | Latest |
| **Adafruit GFX Library** | Graphics rendering (fonts, shapes) | Latest |
| **Adafruit NeoPixel** | WS2812 RGB LED control | Latest |
| **ArduinoJson** | JSON parsing for API responses | Latest |
| **WiFi** | ESP32 built-in WiFi library | Built-in |
| **HTTPClient** | HTTP requests for API calls | Built-in |
| **WebServer** | HTTP server for WiFi portal and OTA | Built-in |
| **Preferences** | NVS (Non-Volatile Storage) for settings | Built-in |
| **PCNT** | Pulse Counter (rotary encoder) | Built-in |

All libraries are automatically downloaded and managed by PlatformIO during the build process.

---

## Adding a Custom Library

If you need to add a project-specific library:

1. Create a subdirectory in `lib/`:
   ```
   lib/
   └── MyCustomLib/
       ├── src/
       │   ├── MyCustomLib.h
       │   └── MyCustomLib.cpp
       └── library.json (optional)
   ```

2. Include in your code:
   ```cpp
   #include <MyCustomLib.h>
   ```

3. PlatformIO's Library Dependency Finder will automatically detect and compile it.

---

## See Also

- **Source code:** `src/README.md` for main application modules
- **Header files:** `include/README.md` for API reference
- **Dependencies:** `platformio.ini` for external library configuration
