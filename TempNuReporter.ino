// TempNuReporter.ino
//
// Utvecklad av A. Isaksson SA7BNB — 2026
//
// Temperaturgivare för temperatur.nu — ESP32-C3 Super Mini + DS18B20, utan display.
// All konfiguration sker via webbsidor.
//
//   1. Första start (eller efter återställning): enheten startar accesspunkten
//      "Temperatur-XXXXXX". Anslut, välj wifi i listan, ange lösenord.
//   2. Därefter nås konfigurationssidan på http://<enhetsnamn>.local/ eller via IP.
//   3. Håll BOOT-knappen (GPIO9) intryckt i 5 sekunder och släpp: inställningarna raderas.
//      Håll kvar i 15 sekunder: hård återställning — enheten får dessutom en ny slumpad
//      identitet (PIN + MAC) och uppträder som en helt ny enhet.
//
// Koppling:  DS18B20 DATA -> GPIO4, VCC -> 3V3, GND -> GND, 4,7 kΩ mellan DATA och 3V3.
//
// Bibliotek (Library Manager):  OneWire (Paul Stoffregen), DallasTemperature (Miles Burton)
// Board: ESP32C3 Dev Module, USB CDC On Boot: Enabled

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <Update.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <esp_wifi.h>
#include "pages.h"

#define FW_VERSION "1.2.0"

// ---------------------------------------------------------------- pinnar
#define ONE_WIRE_PIN 4   // DS18B20 data
#define BUTTON_PIN   9   // BOOT-knappen (aktiv låg)
#define LED_PIN      8   // inbyggd LED (aktiv låg)

// ---------------------------------------------------------------- tider
const unsigned long BOOT_CONNECT_TIMEOUT_MS = 20000;   // väntan på sparat wifi vid start
const unsigned long CONNECT_TIMEOUT_MS      = 20000;   // väntan när man valt nät i portalen
const unsigned long AP_IDLE_RESTART_MS      = 5UL * 60UL * 1000UL;  // AP utan klient -> nytt försök
const unsigned long STA_LOST_RESTART_MS     = 5UL * 60UL * 1000UL;  // wifi borta så länge -> omstart
const unsigned long SENSOR_INTERVAL_MS      = 5000;
const unsigned long SENSOR_CONVERSION_MS    = 800;     // 12 bitar = 750 ms
const unsigned long REPORT_RETRY_MS         = 60000;
const unsigned long FIRST_REPORT_DELAY_MS   = 10000;
const unsigned long RESET_HOLD_MS           = 5000;    // släpp efter 5 s: radera inställningar
const unsigned long HARD_RESET_HOLD_MS      = 15000;   // håll 15 s: ny identitet också

#define MAX_SENSORS 4

// Historik för grafen på konfigurationssidan (ligger i RAM, nollställs vid omstart)
#define HIST_SIZE    120            // 120 värden x 2 min = 4 timmar
#define HIST_STEP_S  120

// ---------------------------------------------------------------- inställningar (NVS)
struct Config {
  String   ssid, pass;      // wifi
  String   name;            // enhetsnamn / hostname
  String   hash;            // hashkod för rapportera.php
  String   admin;           // lösenord för konfigurationssidan (tomt = inget)
  uint8_t  method;          // 0 = PIN (rapportera_v2.php), 1 = hash (rapportera.php)
  uint16_t interval;        // sekunder mellan rapporter
  float    offset;          // kalibrering i °C
  bool     enabled;         // rapportering på/av
  uint8_t  txp;             // 0 = 8,5 dBm, 1 = 15 dBm, 2 = 19,5 dBm
};

Config      cfg;
Preferences prefs;
WebServer   server(80);
DNSServer   dns;
OneWire     oneWire(ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);

// ---------------------------------------------------------------- tillstånd
enum WifiState { WS_IDLE, WS_CONNECTING, WS_CONNECTED, WS_FAILED };

bool          apMode = false;
String        apName;
WifiState     wifiState = WS_IDLE;
String        pendingSsid, pendingPass;
bool          connectRequested = false;
unsigned long connectStart = 0;
volatile uint8_t lastDiscReason = 0;
unsigned long apLastClientMs = 0;
unsigned long staLostSince = 0;
unsigned long restartAt = 0;

float         sensorTemp[MAX_SENSORS];
int           sensorCount = 0;
float         currentTemp = NAN;      // lägsta giltiga värdet (det som rapporteras)

int16_t       hist[HIST_SIZE];        // hundradels grader
int           histCount = 0, histHead = 0;
unsigned long histLastMs = 0;

unsigned long nextReportAt = 0;
unsigned long lastReportMs = 0;
bool          everReported = false;
bool          lastReportOk = false;
String        lastReportMsg;
unsigned long successfulUploads = 1;

uint8_t       idMac[6];               // enhetens identitet (fabriks-MAC eller slumpad)
bool          customIdentity = false;

bool          otaOk = false;
String        otaError;

bool buttonHeldForReset();

// ================================================================ hjälpfunktioner
// Identiteten ligger i ett eget NVS-utrymme ("tnuid") och överlever en vanlig återställning.
// Saknas den används chippets fabriks-MAC. PIN beräknas som i Temperatur.nu Reporter.
void loadIdentity() {
  prefs.begin("tnuid", true);
  customIdentity = prefs.getBytes("mac", idMac, 6) == 6;
  prefs.end();
  if (!customIdentity) {
    uint64_t m = ESP.getEfuseMac();       // mac[i] = byte i
    for (int i = 0; i < 6; i++) idMac[i] = (m >> (8 * i)) & 0xFF;
  }
}

// Slumpar en ny identitet. Adressen märks som "lokalt administrerad" (bit 1 i första byten)
// så att den aldrig krockar med en tillverkares riktiga MAC.
void newIdentity() {
  uint8_t mac[6];
  for (int i = 0; i < 6; i++) mac[i] = esp_random() & 0xFF;
  mac[0] = (mac[0] | 0x02) & 0xFE;
  prefs.begin("tnuid", false);
  prefs.putBytes("mac", mac, 6);
  prefs.end();
}

uint32_t chipId() {
  return ((uint32_t)idMac[3] << 16) | ((uint32_t)idMac[4] << 8) | idMac[5];
}

String macString() {
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
           idMac[0], idMac[1], idMac[2], idMac[3], idMac[4], idMac[5]);
  return String(buf);
}

// Med egen identitet används den även som wifi-MAC, så att routern ser en ny enhet.
void applyWifiMac() {
  if (customIdentity) esp_wifi_set_mac(WIFI_IF_STA, idMac);
}

String chipSuffix() {
  char buf[7];
  snprintf(buf, sizeof(buf), "%06X", (unsigned int)chipId());
  return String(buf);
}

String defaultName() { String s = "temperatur-" + chipSuffix(); s.toLowerCase(); return s; }

// Giltigt hostname: a-z, 0-9 och bindestreck
String sanitizeName(const String& in) {
  String out;
  for (size_t i = 0; i < in.length() && out.length() < 32; i++) {
    char c = tolower(in[i]);
    if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) out += c;
    else if ((c == '-' || c == ' ' || c == '_') && out.length() && out[out.length() - 1] != '-') out += '-';
  }
  while (out.length() && out[out.length() - 1] == '-') out.remove(out.length() - 1);
  return out.length() ? out : defaultName();
}

String jsonEsc(const String& s) {
  String o;
  o.reserve(s.length() + 8);
  for (size_t i = 0; i < s.length(); i++) {
    unsigned char c = s[i];
    if (c == '"' || c == '\\') { o += '\\'; o += (char)c; }
    else if (c < 0x20) { char b[8]; snprintf(b, sizeof(b), "\\u%04x", c); o += b; }
    else o += (char)c;
  }
  return o;
}

String urlEncode(const String& s) {
  String o;
  for (size_t i = 0; i < s.length(); i++) {
    unsigned char c = s[i];
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') o += (char)c;
    else { char b[4]; snprintf(b, sizeof(b), "%%%02X", c); o += b; }
  }
  return o;
}

void applyTxPower() {
  // C3 Super Mini har ofta en dåligt anpassad antenn; låg effekt ger stabilast anslutning.
  wifi_power_t p = cfg.txp == 2 ? WIFI_POWER_19_5dBm : cfg.txp == 1 ? WIFI_POWER_15dBm : WIFI_POWER_8_5dBm;
  WiFi.setTxPower(p);
}

void ledSet(bool on) { digitalWrite(LED_PIN, on ? LOW : HIGH); }

// ================================================================ NVS
void loadConfig() {
  prefs.begin("tnu", true);
  cfg.ssid     = prefs.getString("ssid", "");
  cfg.pass     = prefs.getString("pass", "");
  cfg.name     = prefs.getString("name", defaultName());
  cfg.hash     = prefs.getString("hash", "");
  cfg.admin    = prefs.getString("admin", "");
  cfg.method   = prefs.getUChar("method", 0);
  cfg.interval = prefs.getUShort("interval", 120);
  cfg.offset   = prefs.getFloat("offset", 0.0f);
  cfg.enabled  = prefs.getBool("enabled", true);
  cfg.txp      = prefs.getUChar("txp", 0);
  prefs.end();
}

void saveSettings() {
  prefs.begin("tnu", false);
  prefs.putString("name", cfg.name);
  prefs.putString("hash", cfg.hash);
  prefs.putString("admin", cfg.admin);
  prefs.putUChar("method", cfg.method);
  prefs.putUShort("interval", cfg.interval);
  prefs.putFloat("offset", cfg.offset);
  prefs.putBool("enabled", cfg.enabled);
  prefs.putUChar("txp", cfg.txp);
  prefs.end();
}

void saveWifi(const String& ssid, const String& pass) {
  cfg.ssid = ssid; cfg.pass = pass;
  prefs.begin("tnu", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();
}

void factoryReset() {               // raderar alla inställningar, behåller identiteten
  prefs.begin("tnu", false);
  prefs.clear();
  prefs.end();
}

void hardReset() {                  // raderar allt och ger enheten en ny identitet
  factoryReset();
  newIdentity();
}

// ================================================================ wifi
void onWifiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) lastDiscReason = info.wifi_sta_disconnected.reason;
}

String failReason() {
  switch (lastDiscReason) {
    case 15:   // 4WAY_HANDSHAKE_TIMEOUT
    case 202:  // AUTH_FAIL
    case 204:  // HANDSHAKE_TIMEOUT
    case 2:    // AUTH_EXPIRE
      return "fel lösenord?";
    case 201:  // NO_AP_FOUND
      return "nätverket hittades inte.";
    default:
      return "inget svar från nätverket (orsak " + String(lastDiscReason) + ").";
  }
}

void startAP() {
  apMode = true;
  apName = "Temperatur-" + chipSuffix();
  WiFi.setAutoReconnect(false);
  WiFi.disconnect();                 // STA ska vara tyst, annars fungerar inte sökningen
  WiFi.mode(WIFI_AP_STA);
  applyWifiMac();
  IPAddress ip(192, 168, 4, 1);
  WiFi.softAPConfig(ip, ip, IPAddress(255, 255, 255, 0));
  WiFi.softAP(apName.c_str());
  applyTxPower();
  dns.setErrorReplyCode(DNSReplyCode::NoError);
  dns.start(53, "*", ip);            // alla namn -> oss, så öppnas portalen automatiskt
  apLastClientMs = millis();
  Serial.printf("AP-läge: anslut till \"%s\" och öppna http://192.168.4.1/\n", apName.c_str());
}

bool connectSaved() {
  Serial.printf("Ansluter till \"%s\" ", cfg.ssid.c_str());
  WiFi.mode(WIFI_STA);
  applyWifiMac();
  WiFi.setAutoReconnect(true);
  WiFi.begin(cfg.ssid.c_str(), cfg.pass.length() ? cfg.pass.c_str() : nullptr);
  applyTxPower();
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < BOOT_CONNECT_TIMEOUT_MS) {
    ledSet((millis() / 500) % 2);
    delay(50);
    Serial.print((millis() - start) % 1000 < 50 ? "." : "");
    if (buttonHeldForReset()) return false;   // (startar om själv)
  }
  ledSet(false);
  Serial.println();
  return WiFi.status() == WL_CONNECTED;
}

// ================================================================ knapp
// Håll 5 s och släpp: inställningarna raderas. Håll 15 s: hård återställning (ny identitet).
// LED: snabb blink = håll kvar, fast sken = släpp nu för vanlig återställning.
// Returnerar aldrig true i praktiken, eftersom enheten startar om.
void blinkAck(int n) { for (int i = 0; i < n; i++) { ledSet(true); delay(300); ledSet(false); delay(200); } }

bool buttonHeldForReset() {
  static unsigned long pressStart = 0;
  bool down = digitalRead(BUTTON_PIN) == LOW;
  unsigned long held = pressStart ? millis() - pressStart : 0;

  if (down) {
    if (pressStart == 0) { pressStart = millis(); return false; }
    if (held >= HARD_RESET_HOLD_MS) {
      Serial.println("BOOT hållen i 15 s: hård återställning, ny identitet");
      hardReset();
      blinkAck(6);
      ESP.restart();
      return true;
    }
    if (held >= RESET_HOLD_MS) ledSet(true);
    else if (held > 300) ledSet((millis() / 80) % 2);
    return false;
  }

  pressStart = 0;
  if (held >= RESET_HOLD_MS) {
    Serial.println("BOOT hållen i 5 s: återställer inställningarna");
    factoryReset();
    blinkAck(3);
    ESP.restart();
    return true;
  }
  return false;
}

// ================================================================ sensor
void sensorTask() {
  static unsigned long lastStart = 0, convStart = 0, lastRescan = 0;
  static bool converting = false, first = true;
  unsigned long now = millis();

  if (!converting) {
    if (!first && now - lastStart < SENSOR_INTERVAL_MS) return;
    first = false;
    lastStart = now;
    if (sensors.getDeviceCount() == 0 || now - lastRescan > 60000) {   // hittar sensorer som kopplas in senare
      sensors.begin();
      sensors.setResolution(12);
      lastRescan = now;
    }
    sensors.setWaitForConversion(false);     // blockera inte webbservern under mätningen
    sensors.requestTemperatures();
    convStart = now;
    converting = true;
    return;
  }

  if (now - convStart < SENSOR_CONVERSION_MS) return;
  converting = false;

  int n = sensors.getDeviceCount();
  if (n > MAX_SENSORS) n = MAX_SENSORS;
  float lowest = NAN;
  for (int i = 0; i < n; i++) {
    float t = sensors.getTempCByIndex(i);
    // -127 = ingen kontakt, 85.0 = sensorns startvärde (mätningen blev aldrig klar)
    bool valid = (t > -100.0f) && (t != 85.0f);
    sensorTemp[i] = valid ? t + cfg.offset : NAN;
    if (valid && (isnan(lowest) || sensorTemp[i] < lowest)) lowest = sensorTemp[i];
  }
  sensorCount = n;
  currentTemp = lowest;

  if (!isnan(lowest) && (histCount == 0 || now - histLastMs >= HIST_STEP_S * 1000UL)) {
    histLastMs = now;
    hist[histHead] = (int16_t)lroundf(lowest * 100.0f);
    histHead = (histHead + 1) % HIST_SIZE;
    if (histCount < HIST_SIZE) histCount++;
  }
}

// ================================================================ rapportering
bool sendReport() {
  if (WiFi.status() != WL_CONNECTED) { lastReportMsg = "Inget wifi"; return false; }
  if (isnan(currentTemp))            { lastReportMsg = "Inget giltigt mätvärde från sensorn"; return false; }
  if (cfg.method == 1 && cfg.hash.length() == 0) { lastReportMsg = "Hashkod saknas"; return false; }

  String t = String(currentTemp, 1);
  String url;
  bool https = false;
  if (cfg.method == 1) {
    url = "https://www.temperatur.nu/rapportera.php?hash=" + urlEncode(cfg.hash) + "&t=" + t;
    https = true;
  } else {
    url = "http://report.temperatur.nu/rapportera_v2.php?pin=" + String(chipId())
        + "&mac=" + macString() + "&t=" + t
        + "&n=" + String(sensorCount) + "&u=" + String(successfulUploads);
  }
  Serial.println(url);

  HTTPClient http;
  WiFiClient plain;
  WiFiClientSecure secure;
  http.setConnectTimeout(8000);
  http.setTimeout(8000);
  bool begun;
  if (https) {
    secure.setInsecure();   // ingen certifikatkontroll: bara ett mätvärde skickas, och inget CA-paket behöver underhållas
    begun = http.begin(secure, url);
  } else {
    begun = http.begin(plain, url);
  }
  if (!begun) { lastReportMsg = "Kunde inte öppna anslutningen"; return false; }

  ledSet(true);
  int code = http.GET();
  bool ok = false;
  if (code > 0) {
    String body = http.getString();
    body.trim();
    if (body.length() > 80) body = body.substring(0, 80) + "…";
    lastReportMsg = "HTTP " + String(code) + (body.length() ? ": " + body : "");
    ok = (code == HTTP_CODE_OK);
  } else {
    lastReportMsg = "Fel: " + http.errorToString(code);
  }
  http.end();
  ledSet(false);

  if (ok) successfulUploads++;
  Serial.println(lastReportMsg);
  return ok;
}

void doReport() {
  lastReportOk = sendReport();
  lastReportMs = millis();
  everReported = true;
  nextReportAt = millis() + (lastReportOk ? (unsigned long)cfg.interval * 1000UL : REPORT_RETRY_MS);
}

// ================================================================ webbserver
bool authed() {
  if (cfg.admin.length() == 0) return true;
  if (server.authenticate("admin", cfg.admin.c_str())) return true;
  server.requestAuthentication(BASIC_AUTH, "Temperaturgivare");
  return false;
}

void sendJson(int code, const String& body) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json", body);
}

void sendPage(PGM_P body) {
  server.sendHeader("Cache-Control", "no-store");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html; charset=utf-8", "");
  server.sendContent_P(PAGE_HEAD);
  server.sendContent_P(body);
  server.sendContent("");
}

void handleRoot() {
  if (apMode) { sendPage(PAGE_WIFI); return; }
  if (!authed()) return;
  sendPage(PAGE_CONFIG);
}

void handleConfigPage() {
  if (!authed()) return;
  sendPage(PAGE_CONFIG);
}

// Okända adresser: i AP-läge skickas allt till portalen (captive portal)
void handleNotFound() {
  if (apMode) {
    server.sendHeader("Location", "http://192.168.4.1/", true);
    server.send(302, "text/plain", "");
    return;
  }
  server.send(404, "text/plain", "Finns inte");
}

// --- wifi-portalen (endast i AP-läge) ---
void handleScan() {
  if (!apMode) { sendJson(403, "{\"error\":\"endast i AP-läge\"}"); return; }
  int n = WiFi.scanComplete();
  if (n >= 0) {
    String out = "{\"state\":\"done\",\"nets\":[";
    for (int i = 0; i < n; i++) {
      if (i) out += ',';
      out += "{\"ssid\":\"" + jsonEsc(WiFi.SSID(i)) + "\",\"rssi\":" + String(WiFi.RSSI(i))
           + ",\"open\":" + (WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "true" : "false") + "}";
    }
    out += "]}";
    WiFi.scanDelete();
    sendJson(200, out);
    return;
  }
  if (n == WIFI_SCAN_RUNNING) { sendJson(200, "{\"state\":\"running\"}"); return; }
  // ingen sökning igång
  if (server.hasArg("start") && wifiState != WS_CONNECTING) {
    WiFi.scanNetworks(true /*asynkront*/, false /*dolda*/);
    sendJson(200, "{\"state\":\"running\"}");
    return;
  }
  sendJson(200, "{\"state\":\"idle\"}");
}

void handleConnect() {
  if (!apMode) { sendJson(403, "{\"error\":\"endast i AP-läge\"}"); return; }
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");
  if (ssid.length() == 0 || ssid.length() > 32 || pass.length() > 64) {
    sendJson(400, "{\"ok\":false,\"error\":\"ogiltigt nätverksnamn eller lösenord\"}");
    return;
  }
  pendingSsid = ssid;
  pendingPass = pass;
  connectRequested = true;       // utförs i loop() så att svaret hinner skickas först
  wifiState = WS_CONNECTING;
  sendJson(200, "{\"ok\":true}");
}

void handleWifiStatus() {
  const char* st = wifiState == WS_CONNECTING ? "connecting" : wifiState == WS_CONNECTED ? "connected"
                 : wifiState == WS_FAILED ? "failed" : "idle";
  String out = "{\"state\":\"" + String(st) + "\"";
  out += ",\"name\":\"" + jsonEsc(cfg.name) + "\"";
  out += ",\"ap\":\"" + jsonEsc(apName) + "\"";
  out += ",\"ssid\":\"" + jsonEsc(cfg.ssid) + "\"";
  out += ",\"ip\":\"" + WiFi.localIP().toString() + "\"";
  out += ",\"reason\":\"" + (wifiState == WS_FAILED ? jsonEsc(failReason()) : String("")) + "\"}";
  sendJson(200, out);
}

// --- konfiguration ---
String numOrNull(float v) { return isnan(v) ? String("null") : String(v, 2); }

void handleState() {
  if (!authed()) return;
  String out;
  out.reserve(2048);
  out = "{";
  out += "\"fw\":\"" FW_VERSION "\"";
  out += ",\"name\":\"" + jsonEsc(cfg.name) + "\"";
  out += ",\"ap\":" + String(apMode ? "true" : "false");
  out += ",\"ssid\":\"" + jsonEsc(apMode ? cfg.ssid : WiFi.SSID()) + "\"";
  out += ",\"rssi\":" + String(apMode ? 0 : WiFi.RSSI());
  out += ",\"ip\":\"" + (apMode ? WiFi.softAPIP() : WiFi.localIP()).toString() + "\"";
  out += ",\"pin\":\"" + String(chipId()) + "\"";
  out += ",\"mac\":\"" + macString() + "\"";
  out += ",\"custom_id\":" + String(customIdentity ? "true" : "false");
  out += ",\"uptime\":" + String(millis() / 1000UL);
  out += ",\"temp\":" + numOrNull(currentTemp);
  out += ",\"sensors\":[";
  for (int i = 0; i < sensorCount; i++) { if (i) out += ','; out += numOrNull(sensorTemp[i]); }
  out += "]";
  out += ",\"hist_step\":" + String(HIST_STEP_S) + ",\"hist\":[";
  for (int i = 0; i < histCount; i++) {                   // äldst först
    int idx = (histHead - histCount + i + HIST_SIZE) % HIST_SIZE;
    if (i) out += ',';
    out += String(hist[idx] / 100.0f, 1);
  }
  out += "]";
  long nextIn = (cfg.enabled && !apMode) ? (long)(nextReportAt - millis()) / 1000L : -1;
  if (cfg.enabled && !apMode && nextIn < 0) nextIn = 0;
  out += ",\"next_in\":" + String(nextIn);
  out += ",\"rep_age\":" + (everReported ? String((millis() - lastReportMs) / 1000UL) : String("-1"));
  out += ",\"rep_ok\":" + String(lastReportOk ? "true" : "false");
  out += ",\"rep_msg\":\"" + jsonEsc(lastReportMsg) + "\"";
  out += ",\"enabled\":" + String(cfg.enabled ? "true" : "false");
  out += ",\"method\":" + String(cfg.method);
  out += ",\"hash\":\"" + jsonEsc(cfg.hash) + "\"";
  out += ",\"interval\":" + String(cfg.interval);
  out += ",\"offset\":" + String(cfg.offset, 1);
  out += ",\"txp\":" + String(cfg.txp);
  out += ",\"has_admin\":" + String(cfg.admin.length() ? "true" : "false");
  out += "}";
  sendJson(200, out);
}

void handleSaveConfig() {
  if (!authed()) return;

  String newName = sanitizeName(server.arg("name"));
  bool nameChanged = (newName != cfg.name);

  long iv = server.arg("interval").toInt();
  if (iv < 60) iv = 60;
  if (iv > 1800) iv = 1800;
  float off = server.arg("offset").toFloat();
  if (off < -10.0f || off > 10.0f) { sendJson(400, "{\"ok\":false,\"error\":\"kalibreringen måste vara mellan -10 och 10\"}"); return; }
  String hash = server.arg("hash");
  hash.trim();
  if (hash.length() > 64) { sendJson(400, "{\"ok\":false,\"error\":\"hashkoden är för lång\"}"); return; }
  String admin = server.arg("admin");
  if (admin.length() > 32) { sendJson(400, "{\"ok\":false,\"error\":\"lösenordet är för långt\"}"); return; }

  cfg.name     = newName;
  cfg.enabled  = server.arg("enabled") == "1";
  cfg.method   = server.arg("method") == "1" ? 1 : 0;
  cfg.hash     = hash;
  cfg.interval = (uint16_t)iv;
  cfg.offset   = off;
  int txp = server.arg("txp").toInt();
  cfg.txp      = (txp >= 0 && txp <= 2) ? txp : 0;
  if (server.arg("adminclr") == "1") cfg.admin = "";
  else if (admin.length()) cfg.admin = admin;

  saveSettings();
  applyTxPower();
  if (cfg.enabled && !apMode) nextReportAt = millis() + 3000;   // rapportera snart med nya inställningar

  sendJson(200, "{\"ok\":true,\"restart\":" + String(nameChanged ? "true" : "false")
                + ",\"name\":\"" + jsonEsc(cfg.name) + "\"}");
  if (nameChanged) restartAt = millis() + 1500;                 // hostname sätts vid start
}

void handleReportNow() {
  if (!authed()) return;
  doReport();
  sendJson(200, "{\"ok\":" + String(lastReportOk ? "true" : "false") + ",\"msg\":\"" + jsonEsc(lastReportMsg) + "\"}");
}

void handleRestart() {
  if (!authed()) return;
  sendJson(200, "{\"ok\":true}");
  restartAt = millis() + 1000;
}

void handleResetWifi() {
  if (!authed()) return;
  saveWifi("", "");
  sendJson(200, "{\"ok\":true}");
  restartAt = millis() + 1000;
}

void handleFactory() {
  if (!authed()) return;
  factoryReset();
  sendJson(200, "{\"ok\":true}");
  restartAt = millis() + 1000;
}

void handleHardReset() {
  if (!authed()) return;
  hardReset();
  sendJson(200, "{\"ok\":true}");
  restartAt = millis() + 1000;
}

// --- firmware-uppdatering via webbsidan ---
void handleUpdateDone() {
  if (!authed()) return;
  if (otaOk) {
    sendJson(200, "{\"ok\":true}");
    restartAt = millis() + 1500;
  } else {
    sendJson(200, "{\"ok\":false,\"error\":\"" + jsonEsc(otaError.length() ? otaError : String("okänt fel")) + "\"}");
  }
}

void handleUpdateUpload() {
  HTTPUpload& up = server.upload();
  if (up.status == UPLOAD_FILE_START) {
    otaError = "";
    otaOk = (cfg.admin.length() == 0) || server.authenticate("admin", cfg.admin.c_str());
    if (!otaOk) { otaError = "inte inloggad"; return; }
    Serial.printf("Firmware-uppladdning: %s\n", up.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { otaOk = false; otaError = Update.errorString(); }
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (otaOk && Update.write(up.buf, up.currentSize) != up.currentSize) { otaOk = false; otaError = Update.errorString(); }
  } else if (up.status == UPLOAD_FILE_END) {
    if (otaOk && !Update.end(true)) { otaOk = false; otaError = Update.errorString(); }
  } else if (up.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
    otaOk = false;
    otaError = "uppladdningen avbröts";
  }
}

void setupServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/config", HTTP_GET, handleConfigPage);
  server.on("/api/scan", HTTP_GET, handleScan);
  server.on("/api/connect", HTTP_POST, handleConnect);
  server.on("/api/wifi", HTTP_GET, handleWifiStatus);
  server.on("/api/state", HTTP_GET, handleState);
  server.on("/api/config", HTTP_POST, handleSaveConfig);
  server.on("/api/report", HTTP_POST, handleReportNow);
  server.on("/api/restart", HTTP_POST, handleRestart);
  server.on("/api/resetwifi", HTTP_POST, handleResetWifi);
  server.on("/api/factory", HTTP_POST, handleFactory);
  server.on("/api/hardreset", HTTP_POST, handleHardReset);
  server.on("/update", HTTP_POST, handleUpdateDone, handleUpdateUpload);
  server.onNotFound(handleNotFound);
  server.begin();
}

// ================================================================ AP-lägets tillståndsmaskin
void apTask() {
  dns.processNextRequest();

  if (connectRequested) {
    connectRequested = false;
    WiFi.scanDelete();
    lastDiscReason = 0;
    WiFi.disconnect();
    delay(100);
    Serial.printf("Portal: ansluter till \"%s\"\n", pendingSsid.c_str());
    WiFi.begin(pendingSsid.c_str(), pendingPass.length() ? pendingPass.c_str() : nullptr);
    connectStart = millis();
  } else if (wifiState == WS_CONNECTING) {
    if (WiFi.status() == WL_CONNECTED) {
      saveWifi(pendingSsid, pendingPass);
      wifiState = WS_CONNECTED;
      restartAt = millis() + 12000;      // låt sidan hinna visa IP-adressen, starta sedan om rent i STA-läge
      Serial.printf("Portal: ansluten, IP %s — startar om\n", WiFi.localIP().toString().c_str());
    } else if (millis() - connectStart > CONNECT_TIMEOUT_MS) {
      WiFi.disconnect();
      wifiState = WS_FAILED;
      Serial.printf("Portal: misslyckades (orsak %d)\n", lastDiscReason);
    }
  }

  // Blink: 2 Hz i AP-läge, snabbare under anslutning
  ledSet((millis() / (wifiState == WS_CONNECTING ? 120 : 250)) % 2);

  // Finns ett sparat nät (t.ex. routern var långsam efter strömavbrott): försök igen
  // genom omstart när ingen har varit ansluten till AP:n på ett tag.
  if (WiFi.softAPgetStationNum() > 0 || wifiState == WS_CONNECTING) apLastClientMs = millis();
  if (cfg.ssid.length() && wifiState != WS_CONNECTED && millis() - apLastClientMs > AP_IDLE_RESTART_MS) {
    Serial.println("AP-läge utan klient: startar om och provar sparat wifi igen");
    ESP.restart();
  }
}

// ================================================================ setup / loop
void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  ledSet(false);
  delay(300);
  Serial.printf("\n=== Temperaturgivare för temperatur.nu v%s ===\n", FW_VERSION);

  loadIdentity();
  loadConfig();
  Serial.printf("Namn: %s   PIN: %lu   MAC: %s\n", cfg.name.c_str(), (unsigned long)chipId(), macString().c_str());
  Serial.printf("Identitet: %s\n", customIdentity ? "egen (slumpad vid hård återställning)" : "fabrik");

  sensors.begin();
  sensors.setResolution(12);
  Serial.printf("DS18B20 på GPIO%d: %d st\n", ONE_WIRE_PIN, sensors.getDeviceCount());

  WiFi.persistent(false);               // uppgifterna sparas i vår egen NVS
  WiFi.onEvent(onWifiEvent);
  WiFi.setHostname(cfg.name.c_str());   // måste sättas före WiFi.mode()

  if (cfg.ssid.length() && connectSaved()) {
    Serial.printf("Ansluten. Konfiguration: http://%s/  eller  http://%s.local/\n",
                  WiFi.localIP().toString().c_str(), cfg.name.c_str());
    if (MDNS.begin(cfg.name.c_str())) MDNS.addService("http", "tcp", 80);
    nextReportAt = millis() + FIRST_REPORT_DELAY_MS;
  } else {
    startAP();
  }

  setupServer();
}

void loop() {
  server.handleClient();
  sensorTask();
  buttonHeldForReset();

  if (restartAt && (long)(millis() - restartAt) >= 0) {
    Serial.println("Startar om…");
    delay(100);
    ESP.restart();
  }

  if (apMode) {
    apTask();
  } else {
    if (digitalRead(BUTTON_PIN) == HIGH) ledSet(false);

    // Wifi borta: biblioteket återansluter själv; hjälper inte det startar vi om
    if (WiFi.status() == WL_CONNECTED) {
      staLostSince = 0;
    } else {
      if (staLostSince == 0) staLostSince = millis();
      if (millis() - staLostSince > STA_LOST_RESTART_MS) {
        Serial.println("Wifi borta för länge: startar om");
        ESP.restart();
      }
    }

    if (cfg.enabled && WiFi.status() == WL_CONNECTED && (long)(millis() - nextReportAt) >= 0) {
      if (isnan(currentTemp) && millis() < 30000) {
        nextReportAt = millis() + 2000;      // sensorn har inte hunnit mäta än
      } else {
        doReport();
      }
    }
  }

  delay(5);
}
