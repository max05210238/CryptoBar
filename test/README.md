# CryptoBar Unit Tests (`test/`)

This directory is reserved for PlatformIO unit tests.

**Current status:** CryptoBar does not currently have automated unit tests.

---

## Future Testing Considerations

If you want to add unit tests for CryptoBar:

### 1. **Module Testing Candidates**

Good candidates for unit testing (pure logic, minimal hardware dependencies):

- **`coins.cpp`** - Coin registry lookups
  - Test: `coinIndexByTicker()` finds correct coins
  - Test: Invalid ticker returns -1

- **`day_avg.cpp`** - Day-average calculations
  - Test: Rolling 24h mean with known sample data
  - Test: Cycle mean calculation accuracy

- **`settings_store.cpp`** - NVS load/save
  - Test: Settings round-trip (save → load → verify)
  - Test: Default values when keys missing

- **`app_scheduler.cpp`** - Tick-aligned scheduler
  - Test: Next update time calculation
  - Test: Prefetch window timing

### 2. **Hardware-Dependent Modules (Harder to Test)**

Modules that require hardware or mocking:

- **`encoder_pcnt.cpp`** - Requires actual encoder or PCNT simulation
- **`led_status.cpp`** - Requires WS2812 LED or mock
- **`ui.cpp`** - Requires e-paper display or framebuffer mock
- **`network.cpp`** - Requires network access or API mocks

### 3. **Setting Up Unit Tests**

To add a unit test:

1. Create a test file in `test/`:
   ```cpp
   // test/test_coins.cpp
   #include <unity.h>
   #include "coins.h"

   void test_coinIndexByTicker_btc() {
       int idx = coinIndexByTicker("BTC");
       TEST_ASSERT_EQUAL(0, idx);
   }

   void test_coinIndexByTicker_invalid() {
       int idx = coinIndexByTicker("INVALID");
       TEST_ASSERT_EQUAL(-1, idx);
   }

   void setup() {
       UNITY_BEGIN();
       RUN_TEST(test_coinIndexByTicker_btc);
       RUN_TEST(test_coinIndexByTicker_invalid);
       UNITY_END();
   }

   void loop() {}
   ```

2. Run tests:
   ```bash
   pio test
   ```

3. See results in terminal output.

---

## Testing Strategy Recommendations

For a hardware-focused project like CryptoBar:

1. **Manual testing** - Most reliable for embedded hardware
   - Verify UI on actual e-paper display
   - Test encoder responsiveness with real hardware
   - Check LED animations visually

2. **Integration testing** - Test API fallback chains
   - Verify price fetching works with all sources
   - Check graceful degradation when APIs fail

3. **Unit tests** - For pure logic modules only
   - Coin registry lookups
   - Day-average calculations
   - Settings persistence

4. **Simulation testing** - Use serial monitor
   - Enable verbose debug output
   - Monitor API responses
   - Check scheduler timing

---

## Current Testing Approach

CryptoBar is currently tested via:

- **Serial monitor debugging** - Real-time diagnostics
- **Manual UI testing** - Visual verification on hardware
- **API fallback testing** - Real-world network conditions
- **OTA rollback testing** - Intentional firmware failures

---

## See Also

- **PlatformIO Testing:** https://docs.platformio.org/en/latest/plus/unit-testing.html
- **Unity Testing Framework:** https://github.com/ThrowTheSwitch/Unity
- **Source code:** `src/README.md` for module descriptions
