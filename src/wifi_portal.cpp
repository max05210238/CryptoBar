// CryptoBar V0.97 (Maintenance Status Page)
// WiFi provisioning portal (AP + WebServer) with coin dropdown.
// Stores WiFi creds + coin ticker; optionally accepts additional settings fields.

#include "wifi_portal.h"
#include "coins.h"
#include "config.h"

#include <WiFi.h>
#include <WebServer.h>
#include <string.h>

#ifndef WIFI_SCAN_RUNNING
#define WIFI_SCAN_RUNNING (-1)
#endif
#ifndef WIFI_SCAN_FAILED
#define WIFI_SCAN_FAILED (-2)
#endif

static WebServer s_server(80);

static String s_apSsid;
static bool   s_running   = false;
static bool   s_submitted = false;

static String s_newSsid;
static String s_newPass;
static String s_newCoinTicker;

// Optional settings fields (if provided by portal POST).
static int     s_newUpdPreset = -1;
static int     s_newRfMode    = -1;
static int     s_newBriPreset = -1;
static int     s_newTimeFmt   = -1;
static int     s_newDateFmt   = -1;
static int     s_newDtSize    = -1;
static int     s_newDispCur   = -1;
static int     s_newTzIndex   = -1;
static int     s_newDayAvg    = -1;

static String s_defaultCoinTicker;


// Defaults used to pre-select dropdown values on the portal page.
// Set to -1 to default to "Keep current".
static int s_defRfMode   = 1; // 0=Partial, 1=Full
static int s_defUpd      = 0; // update preset index
static int s_defBri      = 1; // brightness preset index
static int s_defTimeFmt  = 1; // 0=24h, 1=12h
static int s_defDateFmt  = 0; // 0=MM/DD, 1=DD/MM, 2=YYYY-MM-DD
static int s_defDtSize   = 1; // 0=Small, 1=Large
static int s_defCur      = 0; // V0.99f: 0=USD, 1=TWD, 2=EUR, etc. (see CURR_COUNT)
static int s_defTz       = DEFAULT_TIMEZONE_INDEX;
static int s_defDayAvg   = 1; // 0=Off, 1=Rolling 24h, 2=ET 7pm cycle

// V0.99k: Mirror of firmware presets (keep in sync with app_state.cpp)
// 30s, 1min, 3min, 5min, 10min (CoinPaprika supports 30s updates)
static const int kUpdPresetCount = 5;
static const int kUpdPresetSecs[kUpdPresetCount] = {30, 60, 180, 300, 600};
static const char* kUpdPresetLabels[kUpdPresetCount] = {"30s", "1m", "3m", "5m", "10m"};

static void appendOption(String& out, const String& value, const String& label, bool selected) {
  out += "<option value='";
  out += value;
  out += "'";
  if (selected) out += " selected";
  out += ">";
  out += label;
  out += "</option>";
}

void wifiPortalSetDefaultAdvanced(int rfMode, int updPreset, int briPreset,
                                  int timeFmt, int dateFmt, int dtSize,
                                  int dispCur, int tzIndex, int dayAvg) {
 // Allow -1 to mean "Keep current".
  s_defRfMode  = (rfMode == 0 || rfMode == 1) ? rfMode : -1;
  s_defUpd     = (updPreset >= 0 && updPreset < kUpdPresetCount) ? updPreset : -1;
  s_defBri     = (briPreset >= 0 && briPreset <= 2) ? briPreset : -1;
  s_defTimeFmt = (timeFmt == 0 || timeFmt == 1) ? timeFmt : -1;
  s_defDateFmt = (dateFmt >= 0 && dateFmt <= 2) ? dateFmt : -1;
  s_defDtSize  = (dtSize == 0 || dtSize == 1) ? dtSize : -1;
  // V0.99f: Support all currencies (0 to CURR_COUNT-1)
  s_defCur     = (dispCur >= 0 && dispCur < (int)CURR_COUNT) ? dispCur : -1;
  s_defTz      = (tzIndex >= 0 && tzIndex < TIMEZONE_COUNT) ? tzIndex : -1;
  s_defDayAvg  = (dayAvg >= 0 && dayAvg <= 2) ? dayAvg : -1;
}

static String macLast4Hex() {
  String mac = WiFi.macAddress();  // "AA:BB:CC:DD:EE:FF"
  mac.replace(":", "");
  mac.toUpperCase();
  if (mac.length() >= 4) return mac.substring(mac.length() - 4);
  return "XXXX";
}

static String htmlEscape(const String& in) {
  String s = in;
  s.replace("&", "&amp;");
  s.replace("<", "&lt;");
  s.replace(">", "&gt;");
  s.replace("\"", "&quot;");
  s.replace("'", "&#39;");
  return s;
}

static int argToIntOr(const String& v, int fallback) {
  if (v.length() == 0) return fallback;
  return v.toInt();
}

static String jsonEscape(const String& in) {
  String out;
  out.reserve(in.length() + 8);
  for (size_t i = 0; i < in.length(); ++i) {
    char c = in[i];
    switch (c) {
      case '\\': out += "\\\\"; break; // backslash
      case '"':  out += "\\\""; break;  // quote
      case '\n': out += "\\n"; break;
      case '\r': break;
      case '\t': out += "\\t"; break;
      default:
        if ((uint8_t)c < 0x20) out += ' ';
        else out += c;
        break;
    }
  }
  return out;
}

static int rssiToBars(int rssi) {
  if (rssi >= -55) return 5;
  if (rssi >= -65) return 4;
  if (rssi >= -75) return 3;
  if (rssi >= -85) return 2;
  return 1;
}

static const char* channelToBandLabel(int ch) {
 // channel 1-14 => 2.4G, channel 36+ => 5G
  return (ch >= 36) ? "5G" : "2.4G";
}

struct ScanItem {
  String ssid;
  int rssi;
  int ch;
  const char* band;
  int bars;
};

static void handleScanStart() {
 // Start an async scan (non-blocking) to avoid stalling the AP portal.
  int st = WiFi.scanComplete();
  if (st == WIFI_SCAN_RUNNING) {
    s_server.send(200, "application/json", "{\"started\":false,\"running\":true}");
    return;
  }
  WiFi.scanDelete();
  WiFi.scanNetworks(true /*async*/, true /*show_hidden*/);
  s_server.send(200, "application/json", "{\"started\":true,\"running\":true}");
}

static void handleScanJson() {
  int n = WiFi.scanComplete();
  if (n == WIFI_SCAN_RUNNING) {
    s_server.send(200, "application/json", "{\"running\":true,\"items\":[]}");
    return;
  }
  if (n < 0) {
 // WIFI_SCAN_FAILED (-2) or other error.
    s_server.send(200, "application/json", "{\"running\":false,\"items\":[]}");
    return;
  }

  const int kMax = 24;
  ScanItem items[kMax];
  int count = 0;

  for (int i = 0; i < n; ++i) {
    String ssid = WiFi.SSID(i);
    ssid.trim();
    if (ssid.length() == 0) continue;

    int rssi = WiFi.RSSI(i);
    int ch   = WiFi.channel(i);
    const char* band = channelToBandLabel(ch);

 // Dedup by (band, ssid) and keep strongest RSSI.
    int hit = -1;
    for (int j = 0; j < count; ++j) {
      if (items[j].ssid == ssid && strcmp(items[j].band, band) == 0) {
        hit = j;
        break;
      }
    }
    if (hit >= 0) {
      if (rssi > items[hit].rssi) {
        items[hit].rssi = rssi;
        items[hit].ch   = ch;
        items[hit].bars = rssiToBars(rssi);
      }
      continue;
    }

    if (count >= kMax) continue;
    items[count].ssid = ssid;
    items[count].rssi = rssi;
    items[count].ch   = ch;
    items[count].band = band;
    items[count].bars = rssiToBars(rssi);
    count++;
  }

 // Sort by RSSI descending (strongest first)
  for (int i = 0; i < count; ++i) {
    int best = i;
    for (int j = i + 1; j < count; ++j) {
      if (items[j].rssi > items[best].rssi) best = j;
    }
    if (best != i) {
      ScanItem tmp = items[i];
      items[i] = items[best];
      items[best] = tmp;
    }
  }

  String out;
  out.reserve(1280);
  out += "{\"running\":false,\"items\":[";
  for (int i = 0; i < count; ++i) {
    if (i) out += ",";
    out += "{\"ssid\":\"";
    out += jsonEscape(items[i].ssid);
    out += "\",\"band\":\"";
    out += items[i].band;
    out += "\",\"bars\":";
    out += String(items[i].bars);
    out += ",\"rssi\":";
    out += String(items[i].rssi);
    out += "}";
  }
  out += "]}";

  WiFi.scanDelete();
  s_server.send(200, "application/json", out);
}


// Build <option> list for coin dropdown.
static String buildCoinOptionsHtml() {
  String opts;
 // Rough reservation: 20 items * ~40 chars.
  opts.reserve(900);
  const String sel = s_defaultCoinTicker;
  for (int i = 0; i < coinCount(); ++i) {
    const CoinInfo& c = coinAt(i);
    opts += "<option value='";
    opts += c.ticker;
    opts += "'";
    if (sel.length() && sel.equalsIgnoreCase(c.ticker)) {
      opts += " selected";
    }
    opts += ">";
 // Show rank + ticker for easier searching.
    opts += "#";
    opts += String(c.marketRank);
    opts += " ";
    opts += c.ticker;
    opts += "</option>";
  }
  return opts;
}

void wifiPortalSetDefaultCoinTicker(const String& ticker) {
  s_defaultCoinTicker = ticker;
}

void wifiPortalSetDefaultCoinTicker(const char* ticker) {
  if (ticker) s_defaultCoinTicker = String(ticker);
  else s_defaultCoinTicker = String();
}

static void handleRoot() {
  String page;
  page += "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
          "<title>CryptoBar WiFi Setup</title>"
          "<style>"
          "body{font-family:Arial;margin:18px;}"
          "label{display:block;margin-top:10px;}"
          "input,select{width:100%;font-size:16px;padding:10px;margin:8px 0;}"
          "button{font-size:16px;padding:10px 14px;margin:8px 0;}"
          "#ssidRow{display:flex;gap:10px;align-items:center;}"
          "#ssidRow select{flex:1;}"
          "#refreshBtn{width:auto;white-space:nowrap;}"
          "#refreshStatus{min-width:90px;color:#666;font-size:14px;}"
          "</style>"
          "</head><body>";
  page += "<h2>CryptoBar WiFi Setup</h2>";
  page += "<p>Enter your WiFi network SSID and password, then press Save.</p>";
  page += "<form method='POST' action='/save'>";
 // SSID dropdown (populated by scan API) + optional manual fallback
  page += "<label>SSID</label>"
          "<div id='ssidRow'>"
          "<select id='ssidSel' name='ssid' required>"
          "<option value=''>Scanning...</option>"
          "</select>"
          "<button id='refreshBtn' type='button' onclick='refreshList()'>Refresh list</button>"
          "<span id='refreshStatus'></span>"
          "</div>";
  page += "<div id='ssidOtherWrap' style='display:none'>"
          "<label>Other SSID</label><input id='ssidOther' name='ssid_other' placeholder='WiFi SSID'>"
          "</div>";
  page += "<label>Password</label><input name='pass' placeholder='WiFi password' type='text'>";
  page += "<label>Coin</label><select name='coin'>" + buildCoinOptionsHtml() + "</select>";

 // Advanced fields are optional; we accept them on POST if present.
  page += "<details style='margin-top:10px;'><summary>Advanced (optional)</summary>";
page += "<small style='color:#666'>Dropdowns prevent typos. Choose <b>Keep current</b> to leave unchanged.</small>";

// Refresh mode
page += "<label>Refresh Mode (rfMode)</label><select name='rf'>";
appendOption(page, "", "Keep current", s_defRfMode < 0);
appendOption(page, "0", "0 = Partial", s_defRfMode == 0);
appendOption(page, "1", "1 = Full (recommended)", s_defRfMode == 1);
page += "</select>";

// Update preset
page += "<label>Update Preset (upd)</label><select name='upd'>";
appendOption(page, "", "Keep current", s_defUpd < 0);
for (int i = 0; i < kUpdPresetCount; ++i) {
  String label = String(i) + " = " + kUpdPresetLabels[i] + " (" + String(kUpdPresetSecs[i]) + "s)";
  appendOption(page, String(i), label, s_defUpd == i);
}
page += "</select>";

// Brightness preset
page += "<label>Brightness Preset (bri)</label><select name='bri'>";
appendOption(page, "", "Keep current", s_defBri < 0);
appendOption(page, "0", "0 = Low", s_defBri == 0);
appendOption(page, "1", "1 = Medium", s_defBri == 1);
appendOption(page, "2", "2 = High", s_defBri == 2);
page += "</select>";

// Time format
page += "<label>Time Format (tfmt)</label><select name='tfmt'>";
appendOption(page, "", "Keep current", s_defTimeFmt < 0);
appendOption(page, "0", "0 = 24h", s_defTimeFmt == 0);
appendOption(page, "1", "1 = 12h", s_defTimeFmt == 1);
page += "</select>";

// Date format
page += "<label>Date Format (dfmt)</label><select name='dfmt'>";
appendOption(page, "", "Keep current", s_defDateFmt < 0);
appendOption(page, "0", "0 = MM/DD/YYYY", s_defDateFmt == 0);
appendOption(page, "1", "1 = DD/MM/YYYY", s_defDateFmt == 1);
appendOption(page, "2", "2 = YYYY-MM-DD", s_defDateFmt == 2);
page += "</select>";


// Date/Time size
page += "<label>Date/Time Size (dtSize)</label><select name='dtsz'>";
appendOption(page, "", "Keep current", s_defDtSize < 0);
appendOption(page, "0", "0 = Small", s_defDtSize == 0);
appendOption(page, "1", "1 = Large (recommended)", s_defDtSize == 1);
page += "</select>";

// Currency (V0.99f: Multi-currency support)
page += "<label>Currency (cur)</label><select name='cur'>";
appendOption(page, "", "Keep current", s_defCur < 0);
// Generate options for all supported currencies (code only, e.g., "USD")
for (int i = 0; i < (int)CURR_COUNT; i++) {
  String label = String(i) + " = " + CURRENCY_INFO[i].code;
  appendOption(page, String(i), label, s_defCur == i);
}
page += "</select>";

// Timezone
page += "<label>Timezone (tz)</label><select name='tz'>";
appendOption(page, "", "Keep current", s_defTz < 0);
for (int i = 0; i < TIMEZONE_COUNT; ++i) {
  String label = String(i) + " = " + htmlEscape(TIMEZONES[i].label);
  appendOption(page, String(i), label, s_defTz == i);
}
page += "</select>";

// DayAvg mode
page += "<label>DayAvg (dayavg)</label><select name='dayavg'>";
appendOption(page, "", "Keep current", s_defDayAvg < 0);
appendOption(page, "0", "0 = Off", s_defDayAvg == 0);
appendOption(page, "1", "1 = Rolling 24h mean", s_defDayAvg == 1);
appendOption(page, "2", "2 = ET 7pm → 7pm cycle", s_defDayAvg == 2);
page += "</select>";
  page += "</details>";

  page += "<button type='submit'>Save</button>";
  page += "</form>";
  page += "<p style='margin-top:20px;color:#666'>AP: " + htmlEscape(s_apSsid) + "<br>Portal: http://192.168.4.1/</p>";
 // JS: fetch scan results, populate SSID dropdown. Auto-scan now + once again at +20s.
  page += "<script>"
          "const sel=document.getElementById('ssidSel');"
          "const refreshBtn=document.getElementById('refreshBtn');"
          "const refreshStatus=document.getElementById('refreshStatus');"
          "const otherWrap=document.getElementById('ssidOtherWrap');"
          "const otherInput=document.getElementById('ssidOther');"
          "function setStatus(t){refreshStatus.textContent=t||'';}"
          "function setOther(v){otherWrap.style.display=v?'block':'none'; if(!v) otherInput.value='';}"
          "function fmt(it){return '['+it.band+']['+it.bars+'/5] '+it.ssid;}"
          "function populate(items){const prev=sel.value; sel.innerHTML='';"
            "const ph=document.createElement('option'); ph.value=''; ph.textContent='Select WiFi...'; sel.appendChild(ph);"
            "for(const it of items){const o=document.createElement('option'); o.value=it.ssid; o.textContent=fmt(it); sel.appendChild(o);}"
            "const o2=document.createElement('option'); o2.value='__other__'; o2.textContent='Other (type manually)'; sel.appendChild(o2);"
            "if(prev) sel.value=prev; if(sel.value==='__other__') setOther(true);"
          "}"
          "function stopPoll(){if(window._ssidPoll){clearInterval(window._ssidPoll); window._ssidPoll=null;}}"
          "async function scanStart(){"
            "setStatus('Refreshing...');"
            "try{refreshBtn.disabled=true; await fetch('/scan_start');}catch(e){refreshBtn.disabled=false; setStatus('Refresh failed'); setTimeout(()=>setStatus(''),1500);}"
          "}"
          "async function scanPoll(){"
            "try{const r=await fetch('/scan.json'); const j=await r.json();"
              "if(j && j.items) populate(j.items);"
              "if(j && j.running===false){"
                "stopPoll(); refreshBtn.disabled=false; setStatus('Updated'); setTimeout(()=>setStatus(''),1200);"
              "}"
            "}catch(e){stopPoll(); refreshBtn.disabled=false; setStatus('Refresh failed'); setTimeout(()=>setStatus(''),1500);}"
          "}"
          "function startPoll(){stopPoll(); scanPoll(); window._ssidPoll=setInterval(scanPoll,1000);}"
          "function refreshList(){scanStart(); startPoll();}"
          "sel.addEventListener('change',()=>{setOther(sel.value==='__other__');});"
          "refreshList(); setTimeout(()=>{refreshList();},20000);"
          "</script>";

  page += "</body></html>";
  s_server.send(200, "text/html", page);
}

static void handleSave() {
  String ssid = s_server.arg("ssid");
 // If user picked manual SSID entry, use ssid_other.
  if (ssid == "__other__") ssid = s_server.arg("ssid_other");
  String pass = s_server.arg("pass");
  ssid.trim();
  pass.trim();

  String coin = s_server.arg("coin");
  coin.trim();
  if (coin.length() == 0) {
    coin = s_defaultCoinTicker;
  }

  if (ssid.length() == 0) {
    s_server.send(400, "text/plain", "SSID required.");
    return;
  }

 // Optional settings args.
  s_newRfMode    = argToIntOr(s_server.arg("rf"),     -1);
  s_newUpdPreset = argToIntOr(s_server.arg("upd"),    -1);
  s_newBriPreset = argToIntOr(s_server.arg("bri"),    -1);
  s_newTimeFmt   = argToIntOr(s_server.arg("tfmt"),   -1);
  s_newDateFmt   = argToIntOr(s_server.arg("dfmt"),   -1);
  s_newDtSize    = argToIntOr(s_server.arg("dtsz"),   -1);
  s_newDispCur   = argToIntOr(s_server.arg("cur"),    -1);
  s_newTzIndex   = argToIntOr(s_server.arg("tz"),     -1);
  s_newDayAvg    = argToIntOr(s_server.arg("dayavg"), -1);

  s_newSsid = ssid;
  s_newPass = pass;
  s_newCoinTicker = coin;
  s_submitted = true;

  String page;
  page += "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
          "<title>Saved</title></head><body style='font-family:Arial;margin:18px;'>";
  page += "<h2>Saved</h2>";
  page += "<p>Credentials saved. The device is now connecting to:</p>";
  page += "<p><b>" + htmlEscape(ssid) + "</b></p>";
  page += "<p>Coin: <b>" + htmlEscape(coin) + "</b></p>";
  String dtLabel = "Keep current";
  if (s_newDtSize == 0) dtLabel = "Small";
  else if (s_newDtSize == 1) dtLabel = "Large";
  page += "<p>Date/Time Size: <b>" + dtLabel + "</b></p>";
  page += "<p>You may close this page.</p>";
  page += "</body></html>";
  s_server.send(200, "text/html", page);
}

// Reduce noisy "request handler not found" logs from common captive-portal probes.
static void handleFavicon() {
 // Some clients request /favicon.ico; respond with no content.
  s_server.send(204, "image/x-icon", "");
}

static void handleCaptiveProbe() {
 // Redirect common OS captive-portal URLs to the root portal page.
  s_server.sendHeader("Location", "http://192.168.4.1/", true);
  s_server.send(302, "text/plain", "");
}

static void handleNotFound() {
  s_server.sendHeader("Location", "http://192.168.4.1/", true);
  s_server.send(302, "text/plain", "");
}

void wifiPortalStart(const char* apSsid) {
  if (s_running) return;

  if (s_defaultCoinTicker.length() == 0) {
    s_defaultCoinTicker = coinDefault().ticker;
  }

  if (apSsid && apSsid[0]) s_apSsid = String(apSsid);
  else s_apSsid = String("CryptoBar_") + macLast4Hex();

  WiFi.mode(WIFI_AP_STA);
 // : keep WiFi fully awake while running the portal.
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);

  IPAddress ip(192, 168, 4, 1);
  IPAddress gw(192, 168, 4, 1);
  IPAddress sn(255, 255, 255, 0);
  WiFi.softAPConfig(ip, gw, sn);
  WiFi.softAP(s_apSsid.c_str());

  s_server.on("/", HTTP_GET, handleRoot);
  s_server.on("/favicon.ico", HTTP_GET, handleFavicon);
 // Captive portal probe URLs (Android/iOS/macOS/Windows)
  s_server.on("/generate_204", HTTP_GET, handleCaptiveProbe);
  s_server.on("/hotspot-detect.html", HTTP_GET, handleCaptiveProbe);
  s_server.on("/library/test/success.html", HTTP_GET, handleCaptiveProbe);
  s_server.on("/success.txt", HTTP_GET, handleCaptiveProbe);
  s_server.on("/ncsi.txt", HTTP_GET, handleCaptiveProbe);
  s_server.on("/connecttest.txt", HTTP_GET, handleCaptiveProbe);
  s_server.on("/fwlink", HTTP_GET, handleCaptiveProbe);
  s_server.on("/scan_start", HTTP_GET, handleScanStart);
  s_server.on("/scan.json", HTTP_GET, handleScanJson);
  s_server.on("/save", HTTP_POST, handleSave);
  s_server.on("/save", HTTP_GET, handleSave);
  s_server.onNotFound(handleNotFound);
  s_server.begin();

  s_running = true;

  Serial.printf("[WiFi] AP started: %s  IP: %s\n", s_apSsid.c_str(), WiFi.softAPIP().toString().c_str());
}

void wifiPortalStart() {
  wifiPortalStart(nullptr);
}

void wifiPortalStop() {
  if (!s_running) return;
  s_server.stop();
  WiFi.softAPdisconnect(true);
  s_running = false;
  Serial.println("[WiFi] AP stopped");
}

void wifiPortalLoop() {
  if (s_running) s_server.handleClient();
}

bool wifiPortalIsRunning() { return s_running; }

const String& wifiPortalApSsid() { return s_apSsid; }

bool wifiPortalHasSubmission() { return s_submitted; }

bool wifiPortalTakeSubmission(WifiPortalSubmission& out) {
  if (!s_submitted) return false;
  s_submitted = false;

  out.ssid      = s_newSsid;
  out.pass      = s_newPass;
  out.coinTicker = s_newCoinTicker;
  if (out.coinTicker.length() == 0) out.coinTicker = s_defaultCoinTicker;

  out.updPreset = s_newUpdPreset;
  out.rfMode    = s_newRfMode;
  out.briPreset = s_newBriPreset;
  out.timeFmt   = s_newTimeFmt;
  out.dateFmt   = s_newDateFmt;
  out.dtSize    = s_newDtSize;
  out.dispCur   = s_newDispCur;
  out.tzIndex   = s_newTzIndex;
  out.dayAvg    = (s_newDayAvg < 0) ? 0 : (uint8_t)s_newDayAvg;

 // Reset optional values back to "unset" for next submission.
  s_newUpdPreset = -1;
  s_newRfMode    = -1;
  s_newBriPreset = -1;
  s_newTimeFmt   = -1;
  s_newDateFmt   = -1;
  s_newDispCur   = -1;
  s_newTzIndex   = -1;
  s_newDayAvg    = -1;

  return true;
}

bool wifiPortalTakeSubmission(String& outSsid, String& outPass, String& outCoinTicker) {
  WifiPortalSubmission sub;
  if (!wifiPortalTakeSubmission(sub)) return false;
  outSsid = sub.ssid;
  outPass = sub.pass;
  outCoinTicker = sub.coinTicker;
  return true;
}