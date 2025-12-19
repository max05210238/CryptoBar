// CryptoBar V0.97 - Maintenance boot flag
#include "maint_boot.h"

#include <Preferences.h>

static const char* kNs  = "maint_boot";
static const char* kKey = "req"; // bool

void maintBootRequest() {
  Preferences pref;
  if (!pref.begin(kNs, false)) return;
  pref.putBool(kKey, true);
  pref.end();
}

bool maintBootConsumeRequested() {
  Preferences pref;
  if (!pref.begin(kNs, false)) return false;
  bool req = pref.getBool(kKey, false);
  if (req) pref.putBool(kKey, false);
  pref.end();
  return req;
}
