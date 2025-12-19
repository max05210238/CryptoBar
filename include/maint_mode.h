#pragma once

#include <Arduino.h>

// Maintenance mode (firmware update AP)
// : start an open AP + status web page, expose SSID/IP for UI.

void maintModeEnter(const char* version);
void maintModeLoop();
void maintModeExit(bool reboot);

bool maintModeIsActive();
const String& maintModeApSsid();
const String& maintModeApIp();
// : allow UI/main loop to exit maintenance without a reboot.
// Returns true once per user request (consumes the request).
bool maintModeConsumeExitRequest();

