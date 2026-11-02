// ESP32 Antenna Switch - touchscreen + web server
// Based on TechMindsYT/ESP32-Antenna-Switch (AntennaSwitchV1.ino)
// Target: Waveshare ESP32-S3-Touch-LCD-5B (1024x600) + external I2C expander
//
// Architecture
//   * Touch (LVGL task) and web (AsyncTCP task) never touch hardware directly.
//     Both call requestSelect(id), which just records the request.
//   * loop() is the single place that talks to the expander, saves to flash,
//     updates the LVGL screen and pushes the new state to web clients.
//     This avoids I2C / LVGL threading problems and keeps both UIs in sync.

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <Preferences.h>

#include "config.h"
#include "display_port.h"
#include "expander.h"
#include "web_pages.h"
#include "ui.h"

Preferences prefs;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
SemaphoreHandle_t stateMutex;            // guards antennaNames

char antennaNames[NUM_ANT][NAME_LEN + 1];
volatile int  activeAntenna = -1;        // -1 = none selected
volatile int  pendingSelect = -1;        // request from touch or web
volatile bool stateChanged  = false;     // push to web clients + refresh UI
volatile bool uiDirty       = true;      // refresh LVGL only
bool expanderOk    = false;
bool serverStarted = false;

// ---------------------------------------------------------------- helpers
void requestSelect(int id) {             // callable from any task
  if (id >= 0 && id < NUM_ANT) pendingSelect = id;
}

static String jsonEscape(const char *s) {
  String o;
  for (; *s; s++) {
    if (*s == '"' || *s == '\\') { o += '\\'; o += *s; }
    else if ((uint8_t)*s >= 0x20) o += *s;
  }
  return o;
}

static String namesArrayJson() {
  String j = "[";
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  for (int i = 0; i < NUM_ANT; i++) {
    j += "\"" + jsonEscape(antennaNames[i]) + "\"";
    if (i < NUM_ANT - 1) j += ",";
  }
  xSemaphoreGive(stateMutex);
  return j + "]";
}

static String stateJson() {
  String j = "{\"states\":[";
  for (int i = 0; i < NUM_ANT; i++) {
    j += (i == activeAntenna) ? "true" : "false";
    if (i < NUM_ANT - 1) j += ",";
  }
  j += "],\"names\":" + namesArrayJson();
  j += String(",\"io\":") + (expanderOk ? "true" : "false") + "}";
  return j;
}

// ---------------------------------------------------------------- switching
static void applySelection(int id) {
  if (!expanderOk) expanderOk = expBegin();          // retry if it was missing
  if (expanderOk && SWITCH_BREAK_MS > 0) {           // break-before-make
    expWriteOutputs(levelsFor(-1));
    delay(SWITCH_BREAK_MS);
  }
  if (expanderOk) expanderOk = expWriteOutputs(levelsFor(id));
  if (expanderOk) {
    activeAntenna = id;
    prefs.putInt("lastRelay", id);
  }
  stateChanged = true;
}

// ---------------------------------------------------------------- web
static void onRoot(AsyncWebServerRequest *r)     { r->send_P(200, "text/html", index_html); }
static void onSettings(AsyncWebServerRequest *r) { r->send_P(200, "text/html", settings_html); }

static void onGetNames(AsyncWebServerRequest *r) {
  r->send(200, "application/json", "{\"names\":" + namesArrayJson() + "}");
}

static void onSaveNames(AsyncWebServerRequest *r) {
  for (int i = 0; i < NUM_ANT; i++) {
    String key = "name" + String(i);
    if (!r->hasParam(key, true)) continue;
    String n = r->getParam(key, true)->value();
    n.trim();
    if (n.length() == 0) continue;
    if (n.length() > NAME_LEN) n = n.substring(0, NAME_LEN);
    xSemaphoreTake(stateMutex, portMAX_DELAY);
    strlcpy(antennaNames[i], n.c_str(), sizeof(antennaNames[i]));
    xSemaphoreGive(stateMutex);
    prefs.putString(key.c_str(), n);
  }
  stateChanged = true;                               // loop() updates screen + clients
  r->redirect("/settings");
}

static void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (!(info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)) return;
  String msg((const char *)data, len);
  if (msg.indexOf("\"action\":\"toggle\"") < 0) return;
  int p = msg.indexOf("\"id\":");
  if (p < 0) return;
  requestSelect(msg.substring(p + 5).toInt());
}

static void onEvent(AsyncWebSocket *s, AsyncWebSocketClient *c, AwsEventType type,
                    void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT)   c->text(stateJson());
  else if (type == WS_EVT_DATA) handleWebSocketMessage(arg, data, len);
}

static void startServer() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
  server.on("/", HTTP_GET, onRoot);
  server.on("/settings", HTTP_GET, onSettings);
  server.on("/get-names", HTTP_GET, onGetNames);
  server.on("/save-names", HTTP_POST, onSaveNames);
  server.begin();
  Serial.println("Web server started: http://" + WiFi.localIP().toString());
}

// ---------------------------------------------------------------- screen
static void refreshUi() {
  char names[NUM_ANT][NAME_LEN + 1];
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  memcpy(names, antennaNames, sizeof(names));
  xSemaphoreGive(stateMutex);

  String wifiTxt = (WiFi.status() == WL_CONNECTED)
                     ? "http://" + WiFi.localIP().toString()
                     : String("Wi-Fi: connecting...");

  if (lvgl_port_lock(100)) {
    ui_refresh(activeAntenna, names, expanderOk, wifiTxt.c_str());
    lvgl_port_unlock();
  } else {
    uiDirty = true;                                  // try again next pass
  }
}

// ---------------------------------------------------------------- Arduino
void setup() {
  Serial.begin(115200);
  stateMutex = xSemaphoreCreateMutex();

  prefs.begin("antenna", false);                     // same keys as the original
  for (int i = 0; i < NUM_ANT; i++) {
    String def = "Antenna " + String(i + 1);
    String s = prefs.getString(("name" + String(i)).c_str(), def);
    strlcpy(antennaNames[i], s.c_str(), sizeof(antennaNames[i]));
  }

  // 1) Screen + touch (this also brings up the shared I2C bus)
  if (!display_init()) Serial.println("Display/touch init FAILED");
  if (lvgl_port_lock(-1)) { ui_create(); lvgl_port_unlock(); }

  // 2) Expander (after display_init so the I2C driver is already installed)
  expanderOk = expBegin();
  Serial.println(expanderOk ? "I/O expander OK" : "I/O expander NOT FOUND");

  // 3) Restore the last antenna (applied by loop())
  int saved = prefs.getInt("lastRelay", -1);
  if (saved >= 0 && saved < NUM_ANT) pendingSelect = saved;

  // 4) Wi-Fi, non-blocking: the touchscreen works even with no network
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

void loop() {
  ws.cleanupClients();

  static bool lastConnected = false;
  bool connected = (WiFi.status() == WL_CONNECTED);
  if (connected && !serverStarted) { startServer(); serverStarted = true; }
  if (connected != lastConnected)  { lastConnected = connected; uiDirty = true; }

  int sel = pendingSelect;
  if (sel >= 0) {
    pendingSelect = -1;
    if (sel != activeAntenna || !expanderOk) applySelection(sel);
  }

  if (stateChanged) {
    stateChanged = false;
    uiDirty = true;
    if (serverStarted) ws.textAll(stateJson());
  }

  if (uiDirty) { uiDirty = false; refreshUi(); }

  delay(10);
}
