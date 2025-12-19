#pragma once

#include <Arduino.h>

// Maintenance-boot request (V0.97)
//
// Purpose:
// - Switching from STA to AP + WebServer at runtime can be fragile if the
// device is in the middle of network/display work.
// - Instead, request a clean reboot into Maintenance mode using an NVS flag.

// Set the "boot into maintenance" flag in NVS.
void maintBootRequest();

// Consume (read + clear) the maintenance-boot flag.
// Returns true if maintenance mode should be entered this boot.
bool maintBootConsumeRequested();
