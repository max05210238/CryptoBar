#pragma once

#include <Arduino.h>

// OTA safety guard (V0.97)
//
// Goal: reduce the risk of "flash bad firmware -> boot loop -> stuck" when using dual OTA slots.
// Strategy:
// - When Maintenance OTA finishes writing a new image, we mark an "OTA pending" flag in NVS,
// recording the previous (known-good) slot label and the new slot label.
// - On boot, if we're running the "new" slot and the previous boot did not complete verification,
// we count failures. After a small number of failures, we switch boot back to the previous slot.
// - After the firmware has been stable for a short period, we clear the pending flag.

// Call very early in setup(). It may trigger an automatic rollback + reboot.
void otaGuardBootBegin();

// Call frequently in loop(). After stable uptime, it will mark the new firmware as "valid".
void otaGuardLoop();

// Called by maintenance OTA after a successful update (before reboot).
void otaGuardSetPending(const String& prevLabel, const String& newLabel);

// Optional helpers for UI/logging.
bool otaGuardIsPending();
String otaGuardPendingSummary();
