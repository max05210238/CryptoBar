# Pull Request

## Description

**Briefly describe what this PR does:**

## Related Issues

**Link any related issues (will auto-close on merge):**

Fixes #(issue)
Closes #(issue)
Related to #(issue)

## Type of Change

**Select the type of change:**

- [ ] Bug fix (non-breaking change that fixes an issue)
- [ ] New feature (non-breaking change that adds functionality)
- [ ] Breaking change (fix or feature that would cause existing functionality to not work as expected)
- [ ] Documentation update
- [ ] Code refactoring (no functional changes)
- [ ] Performance improvement
- [ ] Hardware modification

## Changes Made

**List the specific changes in this PR:**

-
-
-

## Testing

### Hardware Tested On

**Please specify the hardware you tested on:**

- [ ] ESP32-S3 N16R8 (16MB Flash, 8MB PSRAM)
- [ ] WaveShare 2.9" e-ink display
- [ ] KY-040 Rotary Encoder
- [ ] Other: (specify)

### Test Scenarios

**What did you test?**

- [ ] Fresh boot / first-time setup
- [ ] WiFi connection and reconnection
- [ ] Price updates (all API fallback layers)
- [ ] Display rendering (no artifacts or ghosting)
- [ ] Encoder navigation (menu scrolling, selection)
- [ ] LED indicators (correct colors and animations)
- [ ] OTA firmware update
- [ ] Settings persistence (NVS save/load)
- [ ] Long-term stability (24+ hours)
- [ ] Edge cases (API timeout, network loss, invalid data)

### Test Results

**Summarize test results:**

**Duration:** (e.g., 24 hours continuous operation)
**Update Interval:** (e.g., 3 minutes)
**Network:** (e.g., 2.4GHz WiFi, -65dBm signal)

**Results:**
- ✅ Feature X working as expected
- ✅ No regressions in existing functionality
- ⚠️ Minor issue: (describe) - (acceptable / needs follow-up)

### Serial Monitor Output

**If applicable, paste relevant serial output:**

```
[Paste key serial output here showing successful operation]
```

## Screenshots / Photos

**If UI changes, include before/after screenshots:**

**Before:**
(image or description)

**After:**
(image or description)

## Code Quality Checklist

**Please confirm:**

- [ ] Code compiles without errors (`pio run`)
- [ ] Code compiles without warnings
- [ ] Follows project coding style (see CONTRIBUTING.md)
- [ ] Code is commented where necessary
- [ ] No debug code or commented-out code left in
- [ ] Memory usage is reasonable (no obvious leaks)

## Documentation

**Documentation updated:**

- [ ] README.md (if feature affects user-facing functionality)
- [ ] CHANGELOG.md (added entry for this version)
- [ ] Relevant guide in `docs/guides/` (if needed)
- [ ] Code comments / docstrings
- [ ] No documentation changes needed

**If documentation updated, which files?**

-
-

## Breaking Changes

**Does this PR introduce breaking changes?**

- [ ] No breaking changes
- [ ] Yes, breaking changes (describe below)

**If yes, describe what breaks and migration path:**

## Additional Notes

**Any additional context, concerns, or discussion points:**

## Checklist

**Before submitting, verify:**

- [ ] I have read [CONTRIBUTING.md](../CONTRIBUTING.md)
- [ ] My code follows the project style guidelines
- [ ] I have tested this on real hardware
- [ ] I have updated documentation where needed
- [ ] I have added an entry to CHANGELOG.md (if user-facing)
- [ ] All tests pass and no regressions introduced
- [ ] I have commented my code, particularly in complex areas
- [ ] My changes generate no new warnings
- [ ] I have checked my code for potential security issues

## For Maintainers

**Review focus areas:**

-
-

**Merge instructions:**

- [ ] Squash and merge
- [ ] Regular merge (preserve commits)
- [ ] Rebase and merge
