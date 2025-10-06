// Stats update globals
static bool s_stats_active = false;
static unsigned long s_last_stats_update = 0;
// History buffers for charts
static const int HIST_SIZE = 120; // ~4 minutes at 2s/sample
static float s_cpu_hist[HIST_SIZE];
static float s_ram_hist[HIST_SIZE];
static int s_hist_idx = 0;
static bool s_hist_full = false;

// Disk usage percentage (0-100)
static float s_disk_used_pct = 0.0f;

// Uptime tracking
static uint32_t s_uptime_seconds = 0;
static unsigned long s_last_uptime_tick = 0;
static String s_last_timestamp_iso = ""; // from server metrics

// Layout management
enum LayoutType { LAYOUT_CHARTS = 0, LAYOUT_CLOCK = 1, LAYOUT_MAX = 2 };
static int s_layout = LAYOUT_CHARTS;
static bool s_touch_down = false;

// Helpers for drawing charts
static inline int mapValueToY(float pct, int yTop, int height)
{
  if (pct < 0) pct = 0; if (pct > 100) pct = 100;
  return yTop + (int)((100.0f - pct) * (height / 100.0f));
}

static void drawLineChartArea(int x, int y, int w, int h, const float* data, int size, int idx, bool full, uint16_t color, const char* title)
{
  // Clear area
  lcd.fillRect(x, y, w, h, 0xFFFFu);
  // Border
  lcd.drawRect(x, y, w, h, 0x0000u);
  // Title
  lcd.setTextColor(0x0000u); lcd.setTextSize(1);
  lcd.setCursor(x + 4, y + 2); lcd.print(title);
  // Y-axis grid (25%,50%,75%)
  uint16_t grid = 0xBDF7u; // light gray
  for (int p : {25,50,75}) {
    int gy = mapValueToY((float)p, y + 15, h - 20);
    lcd.drawFastHLine(x + 1, gy, w - 2, grid);
  }
  // Line plot
  int plotY = y + 15; int plotH = h - 20; int plotX = x + 2; int plotW = w - 4;
  int points = full ? size : idx;
  if (points <= 1) return;
  // Iterate across visible window from oldest to newest
  auto getVal = [&](int i)->float{
    if (full) {
      int pos = (idx + i) % size; return data[pos];
    } else {
      return data[i];
    }
  };
  int prevX = plotX;
  int prevY = mapValueToY(getVal(0), plotY, plotH);
  lcd.drawPixel(prevX, prevY, color);
  for (int i = 1; i < (full ? size : idx); ++i) {
    int px = plotX + (i * plotW) / (size - 1);
    int py = mapValueToY(getVal(i), plotY, plotH);
    lcd.drawLine(prevX, prevY, px, py, color);
    prevX = px; prevY = py;
  }
  // Current value label
  float lastPct = getVal((full ? size : idx) - 1);
  lcd.setCursor(x + w - 40, y + 2); lcd.printf("%2.0f%%", lastPct);
}

static void drawStorageBar(int x, int y, int w, int h, float usedPct)
{
  lcd.fillRect(x, y, w, h, 0xFFFFu);
  lcd.drawRect(x, y, w, h, 0x0000u);
  int usedW = (int)(w * (usedPct / 100.0f));
  uint16_t usedColor = 0xF800u; // red
  uint16_t freeColor = 0x07E0u; // green
  if (usedW > 0) lcd.fillRect(x + 1, y + 1, usedW - 2 < 0 ? 0 : usedW - 2, h - 2, usedColor);
  if (usedW < w) lcd.fillRect(x + usedW + 1, y + 1, w - usedW - 2, h - 2, freeColor);
  lcd.setTextColor(0x0000u); lcd.setTextSize(1);
  lcd.setCursor(x + 4, y - 12); lcd.print("Storage");
  lcd.setCursor(x + w - 46, y - 12); lcd.printf("%2.0f%%", usedPct);
}

static void drawUptimeTopRight()
{
  // Render uptime at top-right in a small cleared area
  String up;
  uint32_t s = s_uptime_seconds;
  uint32_t days = s / 86400; s %= 86400;
  uint32_t hours = s / 3600; s %= 3600;
  uint32_t minutes = s / 60; uint32_t seconds = s % 60;
  if (days > 0) up = String(days) + "d " + (hours < 10 ? "0" : "") + String(hours) + ":" + (minutes < 10 ? "0" : "") + String(minutes) + ":" + (seconds < 10 ? "0" : "") + String(seconds);
  else up = (hours < 10 ? "0" : "") + String(hours) + ":" + (minutes < 10 ? "0" : "") + String(minutes) + ":" + (seconds < 10 ? "0" : "") + String(seconds);
  int boxW = 140; int boxH = 14; int x = lcd.width() - boxW - 4; int y = 2;
  lcd.fillRect(x, y, boxW, boxH, 0xFFFFu);
  lcd.setTextColor(0x0000u); lcd.setTextSize(1);
  lcd.setCursor(x + 2, y + 2);
  lcd.print("Up: "); lcd.print(up);
}

static void renderChartsLayout()
{
  // Background and title
  lcd.fillScreen(0xFFFFu);
  lcd.setTextColor(0x0000u); lcd.setTextSize(2);
  lcd.setCursor(6, 4); lcd.print("Sentinel Monitor");
  drawUptimeTopRight();
  // Regions
  int margin = 8;
  int chartW = lcd.width() - 2 * margin;
  int chartH = 60;
  int x = margin;
  int y = 24;
  drawLineChartArea(x, y, chartW, chartH, s_cpu_hist, HIST_SIZE, s_hist_idx, s_hist_full, 0x001Fu, "CPU");
  y += chartH + 10;
  drawLineChartArea(x, y, chartW, chartH, s_ram_hist, HIST_SIZE, s_hist_idx, s_hist_full, 0x07E0u, "RAM");
  y += chartH + 16;
  drawStorageBar(x, y, chartW, 18, s_disk_used_pct);
}

static void renderClockLayout()
{
  // Show a big clock and mini charts
  lcd.fillScreen(0xFFFFu);
  // Derive HH:MM:SS from last ISO timestamp
  String hhmmss = "--:--:--";
  int tpos = s_last_timestamp_iso.indexOf('T');
  if (tpos >= 0 && s_last_timestamp_iso.length() >= tpos + 9) {
    hhmmss = s_last_timestamp_iso.substring(tpos + 1, tpos + 9);
  }
  lcd.setTextColor(0x0000u); lcd.setTextSize(4);
  int16_t tw = lcd.textWidth(hhmmss);
  int cx = (lcd.width() - tw) / 2; int cy = 24;
  lcd.setCursor(cx, cy); lcd.print(hhmmss);
  drawUptimeTopRight();
  // Mini charts below
  int margin = 8; int x = margin; int w = lcd.width() - 2 * margin; int h = 48; int y = 70;
  drawLineChartArea(x, y, w, h, s_cpu_hist, HIST_SIZE, s_hist_idx, s_hist_full, 0x001Fu, "CPU");
  y += h + 8;
  drawLineChartArea(x, y, w, h, s_ram_hist, HIST_SIZE, s_hist_idx, s_hist_full, 0x07E0u, "RAM");
}

static void updateStatsFromServer()
{
  if (s_saved_ip.length() == 0 || s_saved_port.length() == 0 || s_saved_auth.length() == 0) return;
  String url = "http://" + s_saved_ip + ":" + s_saved_port + "/metrics";
  HTTPClient http;
  http.begin(url);
  http.addHeader("Authorization", "Bearer " + s_saved_auth);
  int httpCode = http.GET();
  if (httpCode == 200) {
    String payload = http.getString();
    StaticJsonDocument<1536> doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (!err) {
      // Update histories
      float cpu_pct = doc["cpu"].as<float>();
      float ram_pct = doc["memory"]["percentage"].as<float>();
      float disk_pct = doc["disk"]["percentage"].as<float>();
      s_cpu_hist[s_hist_idx] = cpu_pct;
      s_ram_hist[s_hist_idx] = ram_pct;
      s_disk_used_pct = disk_pct;
      s_hist_idx = (s_hist_idx + 1) % HIST_SIZE;
      if (s_hist_idx == 0) s_hist_full = true;
      // Uptime and timestamp
      s_uptime_seconds = doc["uptime"]["uptime_seconds"].as<uint32_t>();
      s_last_timestamp_iso = doc["timestamp"].as<const char*>();
      // Redraw current layout areas
      if (s_layout == LAYOUT_CHARTS) renderChartsLayout();
      else if (s_layout == LAYOUT_CLOCK) renderClockLayout();
    }
  }
  http.end();
}
static void displayPairingResult(bool success, const String& msg)
{
  lcd.fillScreen(0xFFFFu); // White background
  lcd.setTextColor(success ? 0x07E0u : 0xF800u); // Green for success, red for fail
  lcd.setTextSize(3);
  int16_t textWidth = lcd.textWidth(success ? "Paired!" : "Pairing Failed");
  int16_t x = (lcd.width() - textWidth) / 2;
  int16_t y = lcd.height() / 2 - 40;
  lcd.setCursor(x, y);
  lcd.print(success ? "Paired!" : "Pairing Failed");
  lcd.setTextColor(0x0000u);
  lcd.setTextSize(2);
  textWidth = lcd.textWidth(msg);
  x = (lcd.width() - textWidth) / 2;
  y += 60;
  lcd.setCursor(x, y);
  lcd.print(msg);
  delay(2000);
}

static bool pairWithSentinelServer()
{
  if (s_saved_ip.length() == 0 || s_saved_port.length() == 0 || s_saved_auth.length() == 0) {
    displayPairingResult(false, "Missing config");
    return false;
  }
  String url = "http://" + s_saved_ip + ":" + s_saved_port + "/metrics";
  HTTPClient http;
  http.begin(url);
  http.addHeader("Authorization", "Bearer " + s_saved_auth);
  int httpCode = http.GET();
  if (httpCode == 200) {
    displayPairingResult(true, "Server OK");
    http.end();
    return true;
  } else {
    String err = "HTTP: " + String(httpCode);
    displayPairingResult(false, err);
    http.end();
    return false;
  }
}
#include <Arduino.h>
#include <LovyanGFX.hpp>
#include "generated/logo_png.h"
#include <PNGdec.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

// Use auto-detect config for Sunton CYD 2.8" (ESP32-2432S028)
#include <LGFX_AUTODETECT.hpp>

static LGFX lcd;
static PNG s_png;
static int32_t s_png_x = 0;
static int32_t s_png_y = 0;

// Captive portal globals
static DNSServer s_dns;
static WebServer s_http(80);
static IPAddress s_apIP;
static const byte DNS_PORT = 53;

// Configuration storage
static String s_saved_ssid = "";
static String s_saved_password = "";
static String s_saved_ip = "";
static String s_saved_port = "";
static String s_saved_auth = "";
static bool s_config_complete = false;
static bool s_wifi_connected = false;
static unsigned long s_success_start_time = 0;
static bool s_showing_success = false;

// Forward declarations
static void connectToWiFi();
static void displayWiFiSuccess();
static void displayMainScreen();

static int drawPNGLine(PNGDRAW *pDraw)
{
  // Scale-aware line width from PNGdec
  int w = pDraw->iWidth; // already scaled width
  static uint16_t line[480]; // supports up to 480 px wide lines
  if (w > (int)sizeof(line)/sizeof(line[0])) w = (int)sizeof(line)/sizeof(line[0]);
  // Use BIG_ENDIAN to match LovyanGFX's RGB565 color order (fixes swapped colors)
  s_png.getLineAsRGB565(pDraw, line, PNG_RGB565_BIG_ENDIAN, 0x0000);
  lcd.pushImage(s_png_x, s_png_y + pDraw->y, w, 1, line);
  return 1;
}

void drawBootImage()
{
  Serial.printf("Embedded PNG size: %u bytes\n", (unsigned)g_logo_png_len);
  // Always use PNGdec fallback to avoid driver PNG/rotation quirks
  int rc = s_png.openRAM((uint8_t*)g_logo_png, (uint32_t)g_logo_png_len, drawPNGLine);
  if (rc == PNG_SUCCESS) {
    int32_t w = s_png.getWidth();
    int32_t h = s_png.getHeight();
    // Compute downscale to fit screen (power-of-two scaling)
    int scale = 0;
    while ((w >> scale) > lcd.width() || (h >> scale) > lcd.height()) {
      ++scale;
      if (scale > 4) break; // limit PNGdec scale range
    }
    // Centered position
    int32_t dw = (w >> scale);
    int32_t dh = (h >> scale);
    s_png_x = (lcd.width()  - dw) / 2;
    s_png_y = (lcd.height() - dh) / 2;
    Serial.printf("PNGdec: %ldx%ld scale=%d dst=%ldx%ld at (%ld,%ld)\n", (long)w, (long)h, scale, (long)dw, (long)dh, (long)s_png_x, (long)s_png_y);
    lcd.startWrite();
    lcd.fillScreen(0x000000u);
    int drc = s_png.decode(NULL, scale);
    lcd.endWrite();
    Serial.printf("PNGdec decode rc=%d lastError=%d\n", drc, s_png.getLastError());
    s_png.close();
  } else {
    Serial.printf("PNGdec openFLASH failed: %d\n", rc);
  }
}

// Minimal captive portal: resolve all DNS to AP IP and serve a trivial page
static void handleRoot()
{
  String html = "<!doctype html><html><head><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">";
  html += "<title>Sentinel Setup</title>";
  html += "<style>body{font-family:Arial,sans-serif;margin:20px;background:#f0f0f0}";
  html += ".container{background:white;padding:20px;border-radius:8px;max-width:500px;margin:0 auto}";
  html += ".network{background:#f8f8f8;padding:10px;margin:5px 0;border-radius:4px;cursor:pointer;border:1px solid #ddd}";
  html += ".network:hover{background:#e8e8e8}";
  html += "input,button{width:100%;padding:8px;margin:5px 0;border:1px solid #ccc;border-radius:4px}";
  html += "button{background:#007bff;color:white;cursor:pointer}button:hover{background:#0056b3}";
  html += ".form{display:none;margin-top:20px;padding:15px;background:#f9f9f9;border-radius:4px}";
  html += ".password-toggle{margin:5px 0;font-size:14px}";
  html += ".password-toggle input{width:auto;margin-right:5px}</style></head><body>";
  html += "<div class=\"container\"><h1>Sentinel Configuration</h1>";
  
  if (s_config_complete) {
    html += "<p>✅ Configuration saved successfully!</p>";
    html += "<p><strong>WiFi:</strong> " + s_saved_ssid;
    if (s_wifi_connected) {
      html += " (Connected ✅)<br>";
      html += "<strong>Panel URL:</strong> http://" + WiFi.localIP().toString() + "</p>";
    } else if (s_saved_ssid.length() > 0) {
      html += " (Not connected ❌)</p>";
    } else {
      html += "</p>";
    }
    if (s_saved_ip.length() > 0 && s_saved_port.length() > 0) {
      html += "<p><strong>Server:</strong> " + s_saved_ip + ":" + s_saved_port + "</p>";
    } else {
      html += "<p><strong>Server:</strong> Not configured</p>";
    }
    html += "<button onclick=\"location.href='/reset'\">Reset Configuration</button>";
  } else {
    html += "<h2>Available WiFi Networks</h2>";
    html += "<div id=\"networks\">Scanning...</div>";
    html += "<div id=\"configForm\" class=\"form\">";
    html += "<h3>WiFi Configuration</h3>";
    html += "<input type=\"hidden\" id=\"selectedSSID\">";
    html += "<p><strong>Network:</strong> <span id=\"networkName\"></span></p>";
    html += "<input type=\"password\" id=\"wifiPass\" placeholder=\"WiFi Password\">";
    html += "<div class=\"password-toggle\">";
    html += "<input type=\"checkbox\" id=\"showPassword\" onchange=\"togglePasswordVisibility()\"> ";
    html += "<label for=\"showPassword\">Show password</label>";
    html += "</div>";
    html += "<h3>Server Configuration</h3>";
    html += "<input type=\"text\" id=\"serverIP\" placeholder=\"Server IP Address\">";
    html += "<input type=\"number\" id=\"serverPort\" placeholder=\"Port\">";
    html += "<input type=\"password\" id=\"serverAuth\" placeholder=\"Server Password\">";
    html += "<button onclick=\"saveConfig()\">Save Configuration</button>";
    html += "</div></div>";
    
    html += "<script>";
    html += "function togglePasswordVisibility() {";
    html += "  const field = document.getElementById('wifiPass');";
    html += "  const checkbox = document.getElementById('showPassword');";
    html += "  field.type = checkbox.checked ? 'text' : 'password';";
    html += "}";
    html += "function selectNetwork(ssid) {";
    html += "  document.getElementById('selectedSSID').value = ssid;";
    html += "  document.getElementById('networkName').textContent = ssid;";
    html += "  document.getElementById('configForm').style.display = 'block';";
    html += "}";
    html += "function saveConfig() {";
    html += "  const data = {";
    html += "    ssid: document.getElementById('selectedSSID').value,";
    html += "    password: document.getElementById('wifiPass').value,";
    html += "    ip: document.getElementById('serverIP').value,";
    html += "    port: document.getElementById('serverPort').value,";
    html += "    auth: document.getElementById('serverAuth').value";
    html += "  };";
    html += "  fetch('/save', {method: 'POST', headers: {'Content-Type': 'application/json'}, body: JSON.stringify(data)})";
    html += "    .then(response => response.text())";
    html += "    .then(data => { alert('Configuration saved!'); location.reload(); });";
    html += "}";
    html += "setTimeout(() => { fetch('/scan').then(r => r.text()).then(data => document.getElementById('networks').innerHTML = data); }, 1000);";
    html += "</script>";
  }
  
  html += "</body></html>";
  s_http.send(200, "text/html; charset=UTF-8", html);
}

static void handleScan() 
{
  Serial.println("Scanning for WiFi networks...");
  int n = WiFi.scanNetworks();
  String html = "";
  
  if (n == 0) {
    html = "<p>No networks found</p>";
  } else {
    // Use a set to track unique SSIDs and avoid duplicates
    String seenNetworks[32]; // Max 32 unique networks
    int uniqueCount = 0;
    
    for (int i = 0; i < n && uniqueCount < 32; ++i) {
      String ssid = WiFi.SSID(i);
      
      // Skip empty SSIDs
      if (ssid.length() == 0) continue;
      
      // Check if we've already seen this SSID
      bool isDuplicate = false;
      for (int j = 0; j < uniqueCount; j++) {
        if (seenNetworks[j] == ssid) {
          isDuplicate = true;
          break;
        }
      }
      
      // Only add if it's not a duplicate
      if (!isDuplicate) {
        seenNetworks[uniqueCount++] = ssid;
        
        int rssi = WiFi.RSSI(i);
        String security = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Secured";
        
        html += "<div class=\"network\" onclick=\"selectNetwork('" + ssid + "')\">";
        html += "<strong>" + ssid + "</strong><br>";
        html += "Signal: " + String(rssi) + " dBm | " + security;
        html += "</div>";
      }
    }
  }
  
  s_http.send(200, "text/html", html);
}

static void handleSave()
{
  if (s_http.method() != HTTP_POST) {
    s_http.send(405, "text/plain", "Method not allowed");
    return;
  }
  
  String body = s_http.arg("plain");
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, body);
  
  if (error) {
    s_http.send(400, "text/plain", "Invalid JSON");
    return;
  }
  
  s_saved_ssid = doc["ssid"].as<String>();
  s_saved_password = doc["password"].as<String>();
  s_saved_ip = doc["ip"].as<String>();
  s_saved_port = doc["port"].as<String>();
  s_saved_auth = doc["auth"].as<String>();
  
  // Save to SPIFFS
  JsonDocument configDoc;
  configDoc["ssid"] = s_saved_ssid;
  configDoc["password"] = s_saved_password;
  configDoc["ip"] = s_saved_ip;
  configDoc["port"] = s_saved_port;
  configDoc["auth"] = s_saved_auth;
  
  File configFile = SPIFFS.open("/config.json", "w");
  if (configFile) {
    serializeJson(configDoc, configFile);
    configFile.close();
    s_config_complete = true;
    Serial.println("Configuration saved to SPIFFS");
    Serial.printf("WiFi: %s | Server: %s:%s\n", s_saved_ssid.c_str(), s_saved_ip.c_str(), s_saved_port.c_str());
    
    // If WiFi credentials provided, attempt connection
    if (s_saved_ssid.length() > 0 && s_saved_password.length() >= 0) {
      connectToWiFi();
    }
  } else {
    Serial.println("Failed to save configuration");
  }
  
  s_http.send(200, "text/plain", "OK");
}

static void handleReset()
{
  SPIFFS.remove("/config.json");
  s_saved_ssid = "";
  s_saved_password = "";
  s_saved_ip = "";
  s_saved_port = "";
  s_saved_auth = "";
  s_config_complete = false;
  s_wifi_connected = false;
  Serial.println("Configuration reset");
  
  // Disconnect from WiFi and restart in AP mode
  WiFi.disconnect();
  WiFi.mode(WIFI_AP);
  bool ap_ok = WiFi.softAP("Sentinel");
  s_apIP = WiFi.softAPIP();
  
  // Restart DNS server for captive portal
  s_dns.start(DNS_PORT, "*", s_apIP);
  Serial.printf("Sentinel AP restarted at %s\n", s_apIP.toString().c_str());
  
  // Redraw boot image
  drawBootImage();
  
  s_http.sendHeader("Location", "/");
  s_http.send(302, "text/plain", "");
}

static void loadConfig()
{
  // SPIFFS is already mounted in setup(), so we don't need to mount again
  
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("No saved configuration found");
    return;
  }
  
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();
  
  if (error) {
    Serial.println("Failed to parse configuration");
    return;
  }
  
  s_saved_ssid = doc["ssid"].as<String>();
  s_saved_password = doc["password"].as<String>();
  s_saved_ip = doc["ip"].as<String>();
  s_saved_port = doc["port"].as<String>();
  s_saved_auth = doc["auth"].as<String>();
  s_config_complete = true;
  
  Serial.println("Configuration loaded from SPIFFS");
  Serial.printf("WiFi: %s | Server: %s:%s\n", s_saved_ssid.c_str(), s_saved_ip.c_str(), s_saved_port.c_str());
  
  // Don't attempt WiFi connection here - let setup() handle it
}


static void registerHttpRoutes()
{
  s_http.on("/", HTTP_GET, handleRoot);
  s_http.on("/scan", HTTP_GET, handleScan);
  s_http.on("/save", HTTP_POST, handleSave);
  s_http.on("/reset", HTTP_GET, handleReset);
  // Common OS captive portal probes -> respond with a page (200) to trigger portal UI
  s_http.on("/generate_204", HTTP_ANY, handleRoot);              // Android/Chrome
  s_http.on("/gen_204", HTTP_ANY, handleRoot);                   // Android alt
  s_http.on("/hotspot-detect.html", HTTP_ANY, handleRoot);       // Apple
  s_http.on("/library/test/success.html", HTTP_ANY, handleRoot); // Apple alt
  s_http.on("/ncsi.txt", HTTP_ANY, handleRoot);                  // Windows
  s_http.on("/connecttest.txt", HTTP_ANY, handleRoot);           // Windows alt
  s_http.onNotFound(handleRoot);                                   // Catch-all
}

static void startCaptivePortal()
{
  // DNS: wildcard to our AP IP so any hostname points here
  s_dns.start(DNS_PORT, "*", s_apIP);
  s_http.begin();
  Serial.printf("Captive portal started on AP IP: %s\n", s_apIP.toString().c_str());
}

void setup()
{
  Serial.begin(115200);
  delay(200);
  Serial.println("Booting CYD splash...");
  
  // Display flash storage information
  Serial.println("=== Flash Storage Info ===");
  
  // Program space (sketch size)
  size_t sketchSize = ESP.getSketchSize();
  size_t freeSketchSpace = ESP.getFreeSketchSpace();
  size_t flashChipSize = ESP.getFlashChipSize();
  Serial.printf("Flash Chip: %u bytes (%.1f MB)\n", flashChipSize, flashChipSize / 1024.0 / 1024.0);
  Serial.printf("Sketch: %u bytes (%.1f KB)\n", sketchSize, sketchSize / 1024.0);
  Serial.printf("Free Sketch Space: %u bytes (%.1f KB)\n", freeSketchSpace, freeSketchSpace / 1024.0);
  
  // SPIFFS info (will be available after SPIFFS.begin)
  if (SPIFFS.begin(true)) {
    size_t totalBytes = SPIFFS.totalBytes();
    size_t usedBytes = SPIFFS.usedBytes();
    size_t freeBytes = totalBytes - usedBytes;
    Serial.printf("SPIFFS Total: %u bytes (%.1f KB)\n", totalBytes, totalBytes / 1024.0);
    Serial.printf("SPIFFS Used: %u bytes (%.1f KB)\n", usedBytes, usedBytes / 1024.0);
    Serial.printf("SPIFFS Free: %u bytes (%.1f KB)\n", freeBytes, freeBytes / 1024.0);
  } else {
    Serial.println("SPIFFS mount failed");
  }
  
  // Heap memory info
  Serial.printf("Free Heap: %u bytes (%.1f KB)\n", ESP.getFreeHeap(), ESP.getFreeHeap() / 1024.0);
  Serial.printf("Largest Free Block: %u bytes (%.1f KB)\n", ESP.getMaxAllocHeap(), ESP.getMaxAllocHeap() / 1024.0);
  Serial.println("========================");
  
  // Load saved configuration first
  loadConfig();
  
  // Init display
  delay(100);
  lcd.init();
  lcd.setRotation(1); // some ESP32-2432S028_2-USB batches prefer rotation 0
  lcd.setColorDepth(16);
  lcd.setBrightness(255);
  delay(50);

  // Always show boot image for 2 seconds first
  drawBootImage();
  delay(2000);

  // Register HTTP routes once before any s_http.begin()
  registerHttpRoutes();

  // If we have saved WiFi credentials, try connecting
  if (s_saved_ssid.length() > 0) {
    Serial.println("Found saved WiFi credentials, attempting connection...");
    connectToWiFi();
  }

  // Only start Sentinel AP if WiFi connection failed or no credentials saved
  if (!s_wifi_connected) {
    Serial.println("Starting Sentinel AP mode...");
    WiFi.mode(WIFI_AP);
    bool ap_ok = WiFi.softAP("Sentinel");
    s_apIP = WiFi.softAPIP();
    Serial.printf("SoftAP '%s' %s, IP: %s\n", "Sentinel", ap_ok ? "started" : "FAILED", s_apIP.toString().c_str());

    // Start captive portal services (DNS + HTTP)
    startCaptivePortal();

    // Redraw boot image for AP mode
    drawBootImage();
  }

  // If WiFi connected, success screen is already shown by connectToWiFi()
  // and will automatically transition to main screen after 1.5 seconds
}

static void connectToWiFi()
{
  if (s_saved_ssid.length() == 0) return;
  
  Serial.printf("Attempting to connect to WiFi: %s\n", s_saved_ssid.c_str());
  
  // Switch to STA mode only for WiFi connection
  WiFi.mode(WIFI_STA);
  WiFi.begin(s_saved_ssid.c_str(), s_saved_password.c_str());
  
  // Wait up to 30 seconds for connection
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 60) {
    delay(500);
    attempts++;
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    s_wifi_connected = true;
    IPAddress localIP = WiFi.localIP();
    Serial.printf("\nWiFi connected! IP: %s\n", localIP.toString().c_str());
    Serial.printf("Configuration panel accessible at: http://%s\n", localIP.toString().c_str());
    
    // Stop DNS server (no longer needed without AP)
    s_dns.stop();
    
    // Start HTTP server on WiFi interface
    s_http.begin();
    Serial.printf("HTTP server started on WiFi IP: %s\n", localIP.toString().c_str());
    
  // Always display success message when WiFi connects
  displayWiFiSuccess();
  // Pair with Sentinel-Server if config present
  pairWithSentinelServer();
  } else {
    s_wifi_connected = false;
    Serial.println("\nWiFi connection failed - keeping Sentinel AP active");
    // Go back to AP mode if WiFi connection failed
    WiFi.mode(WIFI_AP);
    bool ap_ok = WiFi.softAP("Sentinel");
    s_apIP = WiFi.softAPIP();
    s_dns.start(DNS_PORT, "*", s_apIP);
    Serial.printf("Sentinel AP restarted at %s\n", s_apIP.toString().c_str());
  }
}

static void displayWiFiSuccess()
{
  // Clear screen with white background
  lcd.fillScreen(0xFFFFu); // White background
  lcd.setTextColor(0x07E0u); // Green text
  lcd.setTextSize(3);
  
  // Center "SUCCESS" text
  int16_t textWidth = lcd.textWidth("SUCCESS");
  int16_t x = (lcd.width() - textWidth) / 2;
  int16_t y = lcd.height() / 2 - 40;
  
  lcd.setCursor(x, y);
  lcd.print("SUCCESS");
  
  // Show connected network name
  lcd.setTextColor(0x0000u); // Black text
  lcd.setTextSize(2);
  String connectedMsg = "Connected to:";
  textWidth = lcd.textWidth(connectedMsg);
  x = (lcd.width() - textWidth) / 2;
  y += 60;
  
  lcd.setCursor(x, y);
  lcd.print(connectedMsg);
  
  // Network name (may need to wrap if too long)
  lcd.setTextSize(2);
  textWidth = lcd.textWidth(s_saved_ssid);
  x = (lcd.width() - textWidth) / 2;
  y += 30;
  
  lcd.setCursor(x, y);
  lcd.print(s_saved_ssid);
  
  // Show IP address and config panel access
  lcd.setTextSize(1);
  String ipMsg = "IP: " + WiFi.localIP().toString();
  textWidth = lcd.textWidth(ipMsg);
  x = (lcd.width() - textWidth) / 2;
  y += 30;
  
  lcd.setCursor(x, y);
  lcd.print(ipMsg);
  
  // Show config panel URL
  lcd.setTextColor(0x07E0u); // Green
  String configMsg = "Config: http://" + WiFi.localIP().toString();
  textWidth = lcd.textWidth(configMsg);
  x = (lcd.width() - textWidth) / 2;
  y += 20;
  
  lcd.setCursor(x, y);
  lcd.print(configMsg);
  
  // Set timer for transition
  s_success_start_time = millis();
  s_showing_success = true;
}

static void displayMainScreen()
{
  // Show charts layout initially
  s_layout = LAYOUT_CHARTS;
  renderChartsLayout();
  s_stats_active = true;
  s_last_stats_update = millis() - 2000; // force immediate update
  updateStatsFromServer();
}

void loop()
{
  // Check if we need to transition from success screen to main screen
  if (s_showing_success && (millis() - s_success_start_time >= 1500)) {
    s_showing_success = false;
    displayMainScreen();
  }

  // Touch to switch layouts
  if (lcd.getTouchRawX() >= 0) {
    // Simple edge-triggered toggle
    if (!s_touch_down) {
      s_touch_down = true;
      s_layout = (s_layout + 1) % LAYOUT_MAX;
      if (s_layout == LAYOUT_CHARTS) renderChartsLayout(); else renderClockLayout();
    }
  } else {
    s_touch_down = false;
  }

  // Periodically update stats if paired
  if (s_stats_active && (millis() - s_last_stats_update >= 2000)) {
    s_last_stats_update = millis();
    updateStatsFromServer();
  }

  // Process captive portal network traffic (only if in AP mode)
  if (!s_wifi_connected) {
    s_dns.processNextRequest();
  }
  s_http.handleClient();
  delay(10);
}
