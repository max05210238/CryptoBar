# Contributing to CryptoBar

Thank you for your interest in contributing to CryptoBar! This document provides guidelines and instructions for contributing to the project.

---

## Table of Contents

1. [Code of Conduct](#code-of-conduct)
2. [Getting Started](#getting-started)
3. [How Can I Contribute?](#how-can-i-contribute)
4. [Reporting Bugs](#reporting-bugs)
5. [Suggesting Features](#suggesting-features)
6. [Code Contributions](#code-contributions)
7. [Pull Request Process](#pull-request-process)
8. [Development Guidelines](#development-guidelines)
9. [Testing](#testing)
10. [Documentation](#documentation)

---

## Code of Conduct

This project adheres to a [Code of Conduct](CODE_OF_CONDUCT.md). By participating, you are expected to uphold this code. Please report unacceptable behavior to the project maintainers.

---

## Getting Started

### Prerequisites

Before contributing, ensure you have:

- [PlatformIO](https://platformio.org/install/ide?install=vscode) installed
- ESP32-S3 development board (for hardware testing)
- Basic knowledge of C++ and Arduino framework
- Git installed and configured

### Setting Up Development Environment

1. **Fork the repository** on GitHub
2. **Clone your fork**:
   ```bash
   git clone https://github.com/YOUR-USERNAME/CryptoBar.git
   cd CryptoBar
   ```
3. **Add upstream remote**:
   ```bash
   git remote add upstream https://github.com/max05210238/CryptoBar.git
   ```
4. **Install dependencies**:
   ```bash
   pio pkg install
   ```
5. **Build the project**:
   ```bash
   pio run
   ```

---

## How Can I Contribute?

### Areas We Welcome Contributions

- **Bug fixes** - Help us squash bugs and improve stability
- **API integrations** - Add support for new cryptocurrency exchanges
- **Hardware support** - Adapt for different display sizes or microcontrollers
- **UI improvements** - Enhance the e-ink display layouts
- **Documentation** - Improve guides, add translations, or clarify instructions
- **Testing** - Help test on different hardware configurations
- **Performance** - Optimize memory usage, API calls, or display refresh

---

## Reporting Bugs

Before reporting a bug, please:

1. **Check existing issues** to avoid duplicates
2. **Update to the latest version** - your bug may already be fixed
3. **Test with minimal configuration** - isolate the issue

### How to Report a Bug

Use the [Bug Report Template](.github/ISSUE_TEMPLATE/bug_report.md) and include:

- **CryptoBar version** (e.g., V0.99r)
- **Hardware configuration** (ESP32-S3 model, display type)
- **Steps to reproduce** the issue
- **Expected behavior** vs actual behavior
- **Serial monitor output** (if applicable)
- **Photos or screenshots** (if UI-related)

**Example:**
```
**Version:** V0.99r
**Hardware:** ESP32-S3 N16R8, WaveShare 2.9" display
**Issue:** Display shows garbled text after 10 minutes
**Steps:**
1. Power on device
2. Wait 10 minutes
3. Display becomes unreadable

**Serial Output:**
[E][network.cpp:123] API timeout after 5000ms
```

---

## Suggesting Features

We love new ideas! Before suggesting a feature:

1. **Check the roadmap** in [PENDING_ITEMS.md](PENDING_ITEMS.md)
2. **Search existing issues** for similar suggestions
3. **Consider the scope** - does it fit CryptoBar's core purpose?

### How to Suggest a Feature

Use the [Feature Request Template](.github/ISSUE_TEMPLATE/feature_request.md) and include:

- **Problem it solves** - Why is this needed?
- **Proposed solution** - How should it work?
- **Alternatives considered** - What other approaches exist?
- **Hardware impact** - Will it require new components?
- **API requirements** - Does it need new data sources?

---

## Code Contributions

### Finding Something to Work On

- Browse [open issues](https://github.com/max05210238/CryptoBar/issues)
- Look for `good first issue` labels for beginner-friendly tasks
- Check [PENDING_ITEMS.md](PENDING_ITEMS.md) for planned features

### Before You Start

1. **Comment on the issue** - Let others know you're working on it
2. **Discuss approach** - For large changes, discuss in the issue first
3. **Create a feature branch**:
   ```bash
   git checkout -b feature/your-feature-name
   ```

---

## Pull Request Process

### 1. Before Submitting

- [ ] Code compiles without warnings (`pio run`)
- [ ] Tested on real hardware (ESP32-S3 + e-ink display)
- [ ] Follows [coding style](#coding-style) guidelines
- [ ] Updates documentation if needed
- [ ] Adds entry to CHANGELOG.md (if user-facing change)

### 2. Commit Messages

Use clear, descriptive commit messages:

**Good:**
```
Fix: WiFi reconnect infinite loop on network timeout

- Add 30s timeout to reconnection attempts
- Implement exponential backoff (2s → 4s → 8s)
- Track g_wifiEverConnected to prevent aggressive retries

Fixes #42
```

**Bad:**
```
fixed wifi bug
```

**Format:**
```
<Type>: <Short description (50 chars max)>

<Detailed explanation (wrap at 72 chars)>
- Bullet points for changes
- Reference issue numbers

Fixes #<issue-number>
```

**Types:** `Fix`, `Add`, `Update`, `Refactor`, `Docs`, `Test`, `Performance`

### 3. Pull Request Template

When creating a PR, use the [Pull Request Template](.github/PULL_REQUEST_TEMPLATE.md) and:

- **Link related issues** - Use "Fixes #123" or "Closes #456"
- **Describe changes** - What and why, not just what
- **Include test results** - Hardware tested on, serial output
- **Add screenshots** - For UI changes

### 4. Review Process

- Maintainers will review your PR within 3-5 days
- Address feedback by pushing new commits (don't force-push)
- Once approved, maintainers will merge your PR
- Your contribution will be credited in release notes

---

## Development Guidelines

### Coding Style

**C++ Style:**
- Use **camelCase** for variables: `currentPrice`, `menuIndex`
- Use **PascalCase** for classes: `NetworkManager`
- Prefix globals with `g_`: `g_currentCoinIndex`, `g_displayCurrency`
- Use **UPPER_CASE** for constants: `DISPLAY_PIN_CS`, `LED_PIN`
- **2-space indentation** (no tabs)
- **Include guards** in headers: `#ifndef FILE_H` / `#define FILE_H`

**Example:**
```cpp
// Global variable
float g_lastPriceUsd = 0.0;

// Constant
#define UPDATE_INTERVAL_MS 180000

// Function
void fetchPriceData() {
    int retryCount = 0;
    for (int i = 0; i < 3; i++) {
        // ...
    }
}
```

### Code Organization

- **Modular design** - Keep functions focused and single-purpose
- **Error handling** - Always check return values and timeouts
- **Comments** - Explain *why*, not *what*
- **Memory safety** - Avoid buffer overflows, check array bounds
- **API limits** - Respect rate limits, implement retry logic

**File Organization:**
```cpp
// app_wifi.cpp structure:
#include <WiFi.h>
#include "app_state.h"

// Function declarations (if needed)
void wifiConnect();

// Function implementations
void wifiConnect() {
    // Implementation
}
```

### Hardware Considerations

- **Test on real hardware** - Emulators can't catch timing issues
- **Check GPIO availability** - ESP32-S3 has different pin restrictions
- **Memory constraints** - ESP32-S3 has 8MB PSRAM, but be mindful
- **API timeout handling** - Network calls can fail or stall
- **E-ink refresh limits** - Minimize full refreshes (display lifespan)

### API Integration Guidelines

When adding new APIs:

1. **Implement fallback** - Don't rely on single source
2. **Add timeout** - Use `client.setTimeout()` (5-10 seconds)
3. **Error handling** - Return false on failure, log errors
4. **Rate limit awareness** - Document API limits in comments
5. **Update fallback chain** - Add to existing cascade in `network.cpp`

**Example:**
```cpp
bool fetchPriceFromNewAPI(const char* coinSymbol, float* outPrice) {
    HTTPClient http;
    http.setTimeout(5000); // 5s timeout

    if (http.GET() != 200) {
        Serial.println("[NewAPI] HTTP request failed");
        return false;
    }

    // Parse response...
    *outPrice = parsedPrice;
    return true;
}
```

---

## Testing

### Manual Testing Checklist

Before submitting a PR, test:

- [ ] **WiFi connection** - First boot portal, reconnection after loss
- [ ] **Price updates** - All 4 API fallback layers
- [ ] **Display rendering** - No artifacts, correct layout
- [ ] **Encoder navigation** - Menu scrolling, selection
- [ ] **LED indicators** - Correct colors for price changes
- [ ] **OTA updates** - Firmware upload works
- [ ] **Settings persistence** - NVS saves/loads correctly
- [ ] **Edge cases** - API timeout, network loss, invalid data

### Hardware Test Configurations

Ideally test on:

- ESP32-S3 N16R8 (primary target)
- WaveShare 2.9" e-ink display
- Different WiFi networks (2.4GHz, weak signal)
- Different update intervals (1min, 3min, 5min, 10min)

### Reporting Test Results

Include in PR description:
```
**Tested On:**
- Hardware: ESP32-S3 N16R8, WaveShare 2.9" display
- WiFi: Home network (2.4GHz, -65dBm)
- Test Duration: 24 hours
- Update Interval: 3 minutes

**Results:**
- ✅ Price updates working (tested all APIs)
- ✅ Display refresh no ghosting
- ⚠️ LED brightness slightly dim on "Low" setting (acceptable)
```

---

## Documentation

### When to Update Documentation

Update docs when you:

- Add a new feature (update README, add guide if needed)
- Change user-facing behavior (update relevant guide)
- Modify API (update DEVELOPER_GUIDE.md)
- Fix a bug (add to CHANGELOG.md)
- Change hardware wiring (update HARDWARE_GUIDE.md)

### Documentation Structure

```
docs/
├── guides/
│   ├── HARDWARE_GUIDE.md      # Hardware assembly
│   ├── DISPLAY_GUIDE.md       # UI and navigation
│   ├── LED_DISPLAY_GUIDE.md   # LED status meanings
│   ├── OTA_UPDATE_GUIDE.md    # Firmware updates
│   ├── SETTINGS_MENU_GUIDE.md # Settings options
│   └── DEVELOPER_GUIDE.md     # Code architecture
└── release-notes/
    └── V0.99x_FEATURE.md      # Version-specific notes
```

### Writing Good Documentation

- **Be specific** - "Press encoder knob for 2 seconds" not "long press"
- **Include examples** - Show real code snippets or screenshots
- **Explain why** - Not just how, but why it works that way
- **Test instructions** - Follow your own guide to verify accuracy
- **Keep it updated** - Outdated docs are worse than no docs

---

## Community

### Getting Help

- **GitHub Discussions** - Ask questions, share builds
- **GitHub Issues** - Report bugs, request features
- **README** - Check existing documentation first

### Recognition

All contributors are credited in:
- Release notes for the version containing their contribution
- GitHub's automatic contributor list
- Special mention for significant contributions

---

## License

By contributing, you agree that your contributions will be licensed under the [MIT License](LICENSE).

---

## Questions?

If you have questions not covered here:

1. Check [existing issues](https://github.com/max05210238/CryptoBar/issues)
2. Start a [discussion](https://github.com/max05210238/CryptoBar/discussions)
3. Contact maintainers via GitHub

---

**Thank you for contributing to CryptoBar!** Your efforts help the maker and crypto communities build better, more affordable tools.
