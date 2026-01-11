#include <Arduino.h>

#include <GxEPD2_4C.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>

#include "ui_list.h"

// e-paper display object (defined in main.cpp)
extern GxEPD2_4C<GxEPD2_290c_GDEY029F51H, GxEPD2_290c_GDEY029F51H::HEIGHT> display;

// Draw a simple vertical scrollbar (shared by list pages).
static void uiDrawScrollbar(int16_t trackX, int16_t trackTop, int16_t trackBottom,
                            int totalItems, int firstIndex, int visibleLines) {
  int trackHeight = trackBottom - trackTop;
  display.drawLine(trackX, trackTop, trackX, trackBottom, GxEPD_BLACK);

  if (totalItems <= visibleLines) {
    display.drawLine(trackX - 1, trackTop, trackX + 1, trackBottom, GxEPD_BLACK);
    return;
  }

  float ratioVisible = (float)visibleLines / (float)totalItems;
  int barHeight = (int)(trackHeight * ratioVisible);
  if (barHeight < 6) barHeight = 6;

  float ratioPos = (float)firstIndex / (float)(totalItems - visibleLines);
  int barTop = trackTop + (int)((trackHeight - barHeight) * ratioPos);

  display.fillRect(trackX - 1, barTop, 3, barHeight, GxEPD_BLACK);
}

void uiDrawListPage(const char* title,
                    const UiListLayout& layout,
                    int totalItems,
                    int firstIndex,
                    int visibleLines,
                    int selectedIndex,
                    UiListPrintItemFn printItem) {
 // Title
  display.setFont(&FreeSansBold12pt7b);
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(layout.x, layout.titleY);
  display.print(title);

 // Items
  display.setFont(&FreeSansBold9pt7b);

  int lastIndex = firstIndex + visibleLines;
  if (lastIndex > totalItems) lastIndex = totalItems;

  for (int i = firstIndex; i < lastIndex; ++i) {
    int y = layout.listYStart + (i - firstIndex) * layout.lineStep;
    display.setCursor(layout.x, y);

    if (i == selectedIndex) display.print("> ");
    else                   display.print("  ");

    if (printItem) printItem(i);
  }

 // Scrollbar
  uiDrawScrollbar(layout.trackX, layout.trackTop, layout.trackBottom,
                  totalItems, firstIndex, visibleLines);
}
