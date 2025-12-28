// CryptoBar V0.98 (Refactored: Step 5)
// app_menu.h - Menu navigation and settings management

#ifndef APP_MENU_H
#define APP_MENU_H

// ==================== Settings Management =====================

// Load all user settings from NVS
void loadSettings();

// Save all user settings to NVS
void saveSettings();

// ==================== Menu Navigation =====================

// Exit menu and return to main price display
void leaveMenu();

// Enter main settings menu
void enterMenu();

// Enter timezone selection submenu
void enterTimezoneSubmenu();

// ==================== Menu Item Handlers =====================

// Handle selection in main menu (cycles through menu items)
void handleMenuSelect();

// Handle timezone selection and apply changes
void handleTimezoneSelect();

// Handle coin selection and refresh data
void handleCoinSelect();

// Handle currency selection and apply changes (V0.99f)
void handleCurrencySelect();

// Handle update interval selection and apply changes (V0.99r)
void handleUpdateIntervalSelect();

#endif // APP_MENU_H
