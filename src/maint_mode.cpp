// CryptoBar V0.97 (Maintenance OTA Upload + OTA safety guard)
#include "maint_mode.h"

#include "ota_guard.h"

#include <WiFi.h>
#include <WebServer.h>

#include <Update.h>

#include <esp_ota_ops.h>
#include <esp_system.h>

static bool   s_active = false;
static String s_version;
static String s_apSsid;
static String s_apIp;

// Reboot is triggered after successful update (or explicit exit). We schedule
// it slightly later so the HTTP response can flush back to the browser.
static bool     s_rebootPending = false;
static uint32_t s_rebootAtMs    = 0;
static bool     s_exitRequested  = false; // exit to normal mode


// OTA upload telemetry (for UI feedback)
static String   s_lastUpdateMsg;
static bool     s_lastUpdateOk   = false;
static size_t   s_otaTotal       = 0;
static size_t   s_otaWritten     = 0;
static String   s_otaFilename;
static String   s_otaTargetLabel;

static WebServer s_server(80);

static String fmtBytes(uint32_t bytes) {
 // Human-ish formatting, keeps UI readable.
  char buf[32];
  if (bytes >= (1024UL * 1024UL)) {
    float mb = (float)bytes / (1024.0f * 1024.0f);
    snprintf(buf, sizeof(buf), "%.2f MB", mb);
  } else if (bytes >= 1024UL) {
    float kb = (float)bytes / 1024.0f;
    snprintf(buf, sizeof(buf), "%.1f KB", kb);
  } else {
    snprintf(buf, sizeof(buf), "%lu B", (unsigned long)bytes);
  }
  return String(buf);
}

static String fmtUptime(uint32_t ms) {
  uint32_t sec = ms / 1000UL;
  uint32_t days = sec / 86400UL;
  sec %= 86400UL;
  uint32_t hrs = sec / 3600UL;
  sec %= 3600UL;
  uint32_t mins = sec / 60UL;
  sec %= 60UL;

  char buf[48];
  if (days > 0) {
    snprintf(buf, sizeof(buf), "%lu d %02lu:%02lu:%02lu",
             (unsigned long)days, (unsigned long)hrs, (unsigned long)mins,
             (unsigned long)sec);
  } else {
    snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu",
             (unsigned long)hrs, (unsigned long)mins, (unsigned long)sec);
  }
  return String(buf);
}

static const char* resetReasonStr(esp_reset_reason_t r) {
  switch (r) {
    case ESP_RST_POWERON:   return "POWERON";
    case ESP_RST_EXT:       return "EXT";
    case ESP_RST_SW:        return "SW";
    case ESP_RST_PANIC:     return "PANIC";
    case ESP_RST_INT_WDT:   return "INT_WDT";
    case ESP_RST_TASK_WDT:  return "TASK_WDT";
    case ESP_RST_WDT:       return "WDT";
    case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";
    case ESP_RST_BROWNOUT:  return "BROWNOUT";
    case ESP_RST_SDIO:      return "SDIO";
    default:                return "UNKNOWN";
  }
}

static String otaLabel(const esp_partition_t* p) {
  if (!p) return String("(none)");
  return String(p->label);
}

static String htmlEscape(const String& s) {
  String out;
  out.reserve(s.length() + 16);
  for (size_t i = 0; i < s.length(); ++i) {
    const char c = s[i];
    switch (c) {
      case '&': out += "&amp;"; break;
      case '<': out += "&lt;";  break;
      case '>': out += "&gt;";  break;
      case '"': out += "&quot;";break;
      case '\'': out += "&#39;"; break;
      default: out += c; break;
    }
  }
  return out;
}

static String jsonEscape(const String& s) {
  String out;
  out.reserve(s.length() + 16);
  for (size_t i = 0; i < s.length(); ++i) {
    const char c = s[i];
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"':  out += "\\\""; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if ((uint8_t)c < 0x20) {
 // Drop other control chars.
        } else {
          out += c;
        }
        break;
    }
  }
  return out;
}

static void scheduleReboot(uint32_t delayMs) {
  s_rebootPending = true;
  s_rebootAtMs = millis() + delayMs;
}

static String macTail4() {
 // Use efuse MAC so we can generate a stable suffix even before WiFi is started.
  uint64_t mac = ESP.getEfuseMac();
  uint16_t tail = (uint16_t)(mac & 0xFFFFULL);
  char buf[5];
  snprintf(buf, sizeof(buf), "%04X", tail);
  return String(buf);
}

static void handleRoot() {
  const esp_partition_t* running = esp_ota_get_running_partition();
  const esp_partition_t* boot    = esp_ota_get_boot_partition();

  const String apSsid = s_apSsid;
  const String apIp   = s_apIp;

 // Status values
  const String fwVer   = htmlEscape(s_version);
 // Note: Arduino-ESP32 2.0.9 (PIO framework-arduinoespressif32 2.0.9) does not
 // ship esp_app_desc.h in the public include path, so we avoid pulling app_desc
 // data here. We still show CryptoBar FW version + build timestamp.
  const String buildTs = String(__DATE__) + " " + String(__TIME__);
  const String uptime  = fmtUptime(millis());
  const String mac     = WiFi.softAPmacAddress();
  const String heap    = fmtBytes((uint32_t)ESP.getFreeHeap());
  const String heapMin = fmtBytes((uint32_t)ESP.getMinFreeHeap());
  const String flashSz = fmtBytes((uint32_t)ESP.getFlashChipSize());
  const uint32_t flashMhz = ESP.getFlashChipSpeed() / 1000000UL;
  const String rst = String(resetReasonStr(esp_reset_reason()));
  const String runningLabel = otaLabel(running);
  const String bootLabel    = otaLabel(boot);

  String html;
  html.reserve(2200);
  html += "<!doctype html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>CryptoBar Maintenance</title>";
  html += "<style>";
  html += "body{font-family:system-ui,-apple-system,Segoe UI,Roboto,Arial; margin:0; padding:18px; background:#f6f7f9;}";
  html += "h1{font-size:20px;margin:0 0 10px 0;}";
  html += ".card{background:#fff;border:1px solid #e5e7eb;border-radius:14px;padding:14px 14px 10px 14px;box-shadow:0 1px 2px rgba(0,0,0,.04);max-width:760px;}";
  html += ".sub{color:#6b7280;font-size:13px;margin:2px 0 12px 0;}";
  html += "table{width:100%;border-collapse:collapse;font-size:14px;}";
  html += "td{padding:8px 6px;border-top:1px solid #f1f3f5;vertical-align:top;}";
  html += "td.k{color:#374151;width:44%;font-weight:600;}";
  html += "td.v{color:#111827;}";
  html += ".pill{display:inline-block;background:#eef2ff;color:#3730a3;border-radius:999px;padding:2px 10px;font-size:12px;margin-left:6px;}";
  html += "a.btn{display:inline-block;margin-top:10px;margin-right:8px;padding:10px 12px;border-radius:12px;border:1px solid #e5e7eb;text-decoration:none;color:#111827;background:#fff;}";
  html += "a.btn.primary{background:#111827;color:#fff;border-color:#111827;}";
  html += "</style></head><body>";

  html += "<div class='card'>";
  html += "<h1>CryptoBar Maintenance</h1>";
  html += "<div class='sub'>Maintenance AP is active. Use this page to confirm device status before updating firmware.";
  html += " <span class='pill'>Step 8</span></div>";
  html += "<table>";
  html += "<tr><td class='k'>Firmware version</td><td class='v'>" + fwVer + "</td></tr>";
 // (App description version omitted on Arduino-ESP32 builds; show FW version + build time instead.)
  html += "<tr><td class='k'>Build time</td><td class='v'>" + htmlEscape(buildTs) + "</td></tr>";
  html += "<tr><td class='k'>Uptime</td><td class='v'>" + htmlEscape(uptime) + "</td></tr>";
  html += "<tr><td class='k'>Reset reason</td><td class='v'>" + htmlEscape(rst) + "</td></tr>";
  html += "<tr><td class='k'>Maintenance SSID</td><td class='v'>" + htmlEscape(apSsid) + "</td></tr>";
  html += "<tr><td class='k'>Maintenance IP</td><td class='v'>" + htmlEscape(apIp) + "</td></tr>";
  html += "<tr><td class='k'>MAC (AP)</td><td class='v'>" + htmlEscape(mac) + "</td></tr>";
  html += "<tr><td class='k'>Free heap</td><td class='v'>" + htmlEscape(heap) + " (min " + htmlEscape(heapMin) + ")</td></tr>";
  html += "<tr><td class='k'>Flash</td><td class='v'>" + htmlEscape(flashSz) + " @ " + String(flashMhz) + " MHz</td></tr>";
  html += "<tr><td class='k'>OTA slot</td><td class='v'>running: <b>" + htmlEscape(runningLabel) + "</b> / boot: <b>" + htmlEscape(bootLabel) + "</b></td></tr>";
 // OTA safety guard is our pragmatic rollback layer (NVS-based) that reduces the risk of getting
 // stuck after flashing a bad image. (Full ESP-IDF rollback can be added later if desired.)
  html += "<tr><td class='k'>OTA safety guard</td><td class='v'>Enabled (2 failed boots on new slot → rollback).</td></tr>";
  html += "</table>";
  html += "<a class='btn primary' href='/update'>Update firmware</a>";
  html += "<a class='btn' href='/exit'>Exit (return to normal)</a><a class='btn' href='/reboot'>Reboot</a>";
  html += "</div></body></html>";

  s_server.send(200, "text/html; charset=utf-8", html);
}

static void sendJson(int statusCode, bool ok, const String& msg) {
  String body;
  body.reserve(256 + msg.length());
  body += "{\"ok\":";
  body += (ok ? "true" : "false");
  body += ",\"msg\":\"";
  body += jsonEscape(msg);
  body += "\"}";
  s_server.send(statusCode, "application/json; charset=utf-8", body);
}

static void handleUpdateGet() {
  const esp_partition_t* running = esp_ota_get_running_partition();
  const esp_partition_t* next    = esp_ota_get_next_update_partition(nullptr);

  const String fwVer = htmlEscape(s_version);
  const String buildTs = String(__DATE__) + " " + String(__TIME__);
  const String runningLabel = htmlEscape(otaLabel(running));
  const String nextLabel    = htmlEscape(otaLabel(next));

  String note;
  if (s_lastUpdateMsg.length() > 0) {
    note = s_lastUpdateOk ? "Last update: OK - " : "Last update: FAIL - ";
    note += s_lastUpdateMsg;
  }

  String html;
  html.reserve(2600);
  html += "<!doctype html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>CryptoBar Firmware Update</title>";
  html += "<style>";
  html += "body{font-family:system-ui,-apple-system,Segoe UI,Roboto,Arial; margin:0; padding:18px; background:#f6f7f9;}";
  html += ".card{background:#fff;border:1px solid #e5e7eb;border-radius:14px;padding:14px 14px 12px 14px;box-shadow:0 1px 2px rgba(0,0,0,.04);max-width:760px;}";
  html += "h1{font-size:20px;margin:0 0 8px 0;}";
  html += ".sub{color:#6b7280;font-size:13px;margin:0 0 12px 0;line-height:1.35;}";
  html += ".row{margin:10px 0;}";
  html += "input[type=file]{width:100%;padding:10px;border:1px dashed #cbd5e1;border-radius:12px;background:#fff;}";
  html += "button{width:100%;padding:12px;border-radius:12px;border:0;background:#111827;color:#fff;font-size:16px;font-weight:700;}";
  html += "button:disabled{background:#9ca3af;}";
  html += ".meta{font-size:13px;color:#374151;margin:6px 0 0 0;}";
  html += ".kv{font-size:13px;color:#374151;margin-top:8px;}";
  html += ".kv b{color:#111827;}";
  html += ".barwrap{height:12px;background:#eef2f7;border-radius:999px;overflow:hidden;margin-top:10px;}";
  html += ".bar{height:12px;width:0%;background:#111827;}";
  html += ".status{margin-top:10px;font-size:14px;color:#111827;min-height:18px;}";
  html += "a.btn{display:inline-block;margin-top:12px;margin-right:8px;padding:10px 12px;border-radius:12px;border:1px solid #e5e7eb;text-decoration:none;color:#111827;background:#fff;}";
  html += "a.btn.primary{background:#111827;color:#fff;border-color:#111827;}";
  html += "</style></head><body>";
  html += "<div class='card'>";
  html += "<h1>Firmware Update</h1>";
  html += "<div class='sub'>Upload a <b>.bin</b> firmware file from your phone. The device will write it to the inactive OTA slot and reboot if successful.";
  if (note.length() > 0) {
    html += "<br><span style='color:#111827'><b>" + htmlEscape(note) + "</b></span>";
  }
  html += "</div>";
  html += "<div class='kv'>Current: <b>" + fwVer + "</b> (" + htmlEscape(buildTs) + ")</div>";
  html += "<div class='kv'>OTA slot: running <b>" + runningLabel + "</b> → target <b>" + nextLabel + "</b></div>";

  html += "<form id='f' class='row'>";
  html += "<input id='fw' name='firmware' type='file' accept='.bin'>";
  html += "<div class='barwrap'><div id='bar' class='bar'></div></div>";
  html += "<div id='status' class='status'></div>";
  html += "<button id='btn' type='submit' disabled>Upload & Update</button>";
  html += "<div class='meta'>Tip: keep this page open until it says rebooting.</div>";
  html += "</form>";

  html += "<a class='btn' href='/'>Back to Status</a>";
  html += "<a class='btn' href='/exit'>Exit (return to normal)</a><a class='btn' href='/reboot'>Reboot</a>";
  html += "</div>";

  html += "<script>";
  html += "const fw=document.getElementById('fw');";
  html += "const btn=document.getElementById('btn');";
  html += "const st=document.getElementById('status');";
  html += "const bar=document.getElementById('bar');";
  html += "fw.addEventListener('change',()=>{btn.disabled=!fw.files||fw.files.length===0;});";
  html += "document.getElementById('f').addEventListener('submit', (e)=>{e.preventDefault(); if(!fw.files||fw.files.length===0)return;";
  html += "btn.disabled=true; st.textContent='Uploading...'; bar.style.width='0%';";
  html += "const xhr=new XMLHttpRequest(); xhr.open('POST','/update');";
  html += "xhr.upload.onprogress=(ev)=>{if(ev.lengthComputable){const p=Math.floor((ev.loaded/ev.total)*100); bar.style.width=p+'%'; st.textContent='Uploading... '+p+'%';}};";
  html += "xhr.onreadystatechange=()=>{if(xhr.readyState===4){let msg=''; try{const r=JSON.parse(xhr.responseText); msg=r.msg||''; if(r.ok){st.textContent=msg||'Update OK. Rebooting...'; bar.style.width='100%';}else{st.textContent='Update failed: '+(msg||('HTTP '+xhr.status)); btn.disabled=false;}}catch(ex){st.textContent='Update failed.'; btn.disabled=false;}}};";
  html += "const fd=new FormData(); fd.append('firmware', fw.files[0]); xhr.send(fd);";
  html += "});";
  html += "</script></body></html>";

  s_server.send(200, "text/html; charset=utf-8", html);
}

static void handleUpdatePostDone() {
  if (s_lastUpdateOk) {
    sendJson(200, true, String("Update OK. Rebooting..."));
    scheduleReboot(900);
  } else {
    const String msg = (s_lastUpdateMsg.length() > 0) ? s_lastUpdateMsg : String("Update failed");
    sendJson(500, false, msg);
  }
}

static void handleUpdateUpload() {
  HTTPUpload& upload = s_server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    s_lastUpdateOk = false;
    s_lastUpdateMsg = "";
    s_otaWritten = 0;
    s_otaTotal = upload.totalSize;
    s_otaFilename = upload.filename;

 // Basic sanity check
    if (!s_otaFilename.endsWith(".bin")) {
      s_lastUpdateMsg = "Invalid file. Please upload a .bin firmware file.";
      return;
    }

    const esp_partition_t* next = esp_ota_get_next_update_partition(nullptr);
    s_otaTargetLabel = otaLabel(next);

    if (!next) {
      s_lastUpdateMsg = "No OTA target partition";
      return;
    }

 // Begin writing to the next OTA partition.
    if (!Update.begin(next->size, U_FLASH)) {
      s_lastUpdateMsg = String("Update.begin failed: ") + String(Update.errorString());
      return;
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (!Update.isRunning() || Update.hasError()) return;
    size_t written = Update.write(upload.buf, upload.currentSize);
    s_otaWritten += written;
    if (written != upload.currentSize) {
      s_lastUpdateMsg = String("Update.write failed: ") + String(Update.errorString());
    }
    yield();
  } else if (upload.status == UPLOAD_FILE_END) {
    if (!Update.isRunning()) {
 // Begin failed earlier or we aborted.
      if (s_lastUpdateMsg.length() == 0) s_lastUpdateMsg = "Update did not start";
      s_lastUpdateOk = false;
      return;
    }

    if (!Update.hasError() && Update.end(true)) {
      s_lastUpdateOk = true;
      s_lastUpdateMsg = "OK";

 // OTA safety guard: remember which slot we came from, so the next boot
 // can roll back automatically if the new image keeps rebooting.
      const esp_partition_t* running = esp_ota_get_running_partition();
      const String prevLabel = otaLabel(running);
      const String newLabel  = s_otaTargetLabel;
      if (prevLabel.length() > 0 && newLabel.length() > 0) {
        otaGuardSetPending(prevLabel, newLabel);
      }
    } else {
      if (s_lastUpdateMsg.length() == 0) {
        s_lastUpdateMsg = String("Update.end failed: ") + String(Update.errorString());
      }
      if (Update.isRunning()) Update.abort();
      s_lastUpdateOk = false;
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    if (Update.isRunning()) Update.abort();
    s_lastUpdateOk = false;
    s_lastUpdateMsg = "Upload aborted";
  }
}

static void handleExit() {
  Serial.println("[MAINT] Exit requested (return to normal mode)");
  s_exitRequested = true;
  s_server.send(200, "text/plain", "Exiting maintenance mode...\n");
}

static void handleReboot() {
  Serial.println("[MAINT] Reboot requested");
  s_server.send(200, "text/plain", "Rebooting...\n");
  scheduleReboot(300);
}

static void handleNotFound() {
  s_server.send(404, "text/plain", "Not found");
}

static void handleNoContent() {
 // Quiet common captive-portal probes and favicon requests.
  s_server.send(204, "text/plain", "");
}

void maintModeEnter(const char* version) {
  if (s_active) return;

  s_version = (version && version[0]) ? String(version) : String("");

 // Start open AP
  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  delay(50);

  s_apSsid = String("CryptoBar_MAINT_") + macTail4();
  WiFi.softAP(s_apSsid.c_str());  // open (no password)
  delay(80);
  s_apIp = WiFi.softAPIP().toString();

 // : status page + OTA upload UI.
  s_server.on("/", handleRoot);
  s_server.on("/update", HTTP_GET, handleUpdateGet);
  s_server.on("/update", HTTP_POST, handleUpdatePostDone, handleUpdateUpload);
  s_server.on("/exit", handleExit);
  s_server.on("/reboot", handleReboot);
 // Common OS captive portal paths (keeps Serial log clean)
  s_server.on("/favicon.ico", handleNoContent);
  s_server.on("/generate_204", handleNoContent);
  s_server.on("/hotspot-detect.html", handleNoContent);
  s_server.on("/library/test/success.html", handleNoContent);
  s_server.on("/success.txt", handleNoContent);
  s_server.on("/ncsi.txt", handleNoContent);
  s_server.on("/connecttest.txt", handleNoContent);
  s_server.on("/fwlink", handleNoContent);
  s_server.onNotFound(handleNotFound);
  s_server.begin();

  s_active = true;
}

void maintModeLoop() {
  if (!s_active) return;
  s_server.handleClient();

  if (s_rebootPending && (int32_t)(millis() - s_rebootAtMs) >= 0) {
    ESP.restart();
  }
}

void maintModeExit(bool reboot) {
  if (!s_active) {
    if (reboot) ESP.restart();
    return;
  }

  s_server.stop();
  delay(50);
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(100);
  s_active = false;

  if (reboot) ESP.restart();
}

bool maintModeIsActive() {
  return s_active;
}

bool maintModeConsumeExitRequest() {
  const bool r = s_exitRequested;
  s_exitRequested = false;
  return r;
}

const String& maintModeApSsid() {
  return s_apSsid;
}

const String& maintModeApIp() {
  return s_apIp;
}
