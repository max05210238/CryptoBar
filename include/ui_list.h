#pragma once

#include <Arduino.h>

// Simple list-page renderer for e-paper UI (used by settings subpages).
// Rendering is done directly to the global 'display' object.

struct UiListLayout {
  int x;           // left margin
  int titleY;      // title baseline Y
  int listYStart;  // first line baseline Y
  int lineStep;    // baseline delta per line
  int trackX;      // scrollbar track X
  int trackTop;    // scrollbar top Y
  int trackBottom; // scrollbar bottom Y
};

// Callback used by uiDrawListPage to print one item (after the prefix "> " / " ").
// The callback should ONLY write the label/value via display.print().
typedef void (*UiListPrintItemFn)(int index);

void uiDrawListPage(const char* title,
                    const UiListLayout& layout,
                    int totalItems,
                    int firstIndex,
                    int visibleLines,
                    int selectedIndex,
                    UiListPrintItemFn printItem);
