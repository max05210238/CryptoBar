// CryptoBar V0.99r - Hardware Display Detection
// Automatically detects E-ink or VFD display using BUSY pin
#pragma once

#include <Arduino.h>
#include "display_interface.h"

// Detect display type by BUSY pin behavior
// Returns: DISPLAY_EINK if E-ink detected, DISPLAY_VFD if VFD detected
DisplayType detectDisplayType();

// Test if E-ink display is connected by checking BUSY pin
// E-ink has active BUSY signal, VFD has floating BUSY pin
bool detectEinkByBusy();
