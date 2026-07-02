/*
  Home Switches - Local ESP8266 Relay Controller
  ------------------------------------------------
  - Controls 10 relays using 2x 74HC595 shift registers (only 3 GPIO pins used)
  - Fully local: no cloud, no Arduino IoT Cloud, no external server
  - Access via http://homeswitches.local (mDNS) - no need to find the IP
  - Simple web dashboard with live status + on/off buttons
  - Relay states are saved to EEPROM so they survive a power cut/reboot
  - Every toggle is logged to Serial Monitor for checking purposes

  WIRING (NodeMCU / ESP8266):
    D7 (GPIO13) -> 74HC595 #1 DS   (data)
    D5 (GPIO14) -> 74HC595 #1 SH_CP (clock)   (also wire SH_CP of #2 to same pin)
    D6 (GPIO12) -> 74HC595 #1 ST_CP (latch)   (also wire ST_CP of #2 to same pin)
    74HC595 #1 Q7' (pin 9) -> 74HC595 #2 DS
    Each 595 output pin -> one relay module's IN pin

  NOTE: Most cheap relay boards are ACTIVE LOW (LOW = relay ON).
  If your relays turn ON when they should be OFF, change ACTIVE_LOW to false below.
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>

// ---------- CONFIG ----------
const char* ssid     = " your ssid";
const char* password = "password";
const char* HOSTNAME = "homeswitches";   // access via http://homeswitches.local

#define NUM_RELAYS 10
bool ACTIVE_LOW = true;   // set false if your relay board is active-HIGH

// Relay names shown on the dashboard - edit to match your rooms/switches
const char* relayNames[NUM_RELAYS] = {
  "Living Room Light", "Hall Light", "Kitchen Light", "Bedroom Light",
  "Bedroom Fan", "Porch Light", "Garden Light", "Water Pump",
  "Outlet 1", "Outlet 2"
};

// ---------- SHIFT REGISTER PINS ----------
#define DS_PIN   13  // D7
#define SHCP_PIN 14  // D5
#define STCP_PIN 12  // D6

bool relayState[NUM_RELAYS] = {false};
ESP8266WebServer server(80);

// ---------- SHIFT REGISTER WRITE ----------
void writeShiftRegisters() {
  digitalWrite(STCP_PIN, LOW);
  for (int i = 15; i >= 0; i--) {
    digitalWrite(SHCP_PIN, LOW);
    bool on = (i < NUM_RELAYS) ? relayState[i] : false;
    bool pinLevel = ACTIVE_LOW ? !on : on;
    digitalWrite(DS_PIN, pinLevel ? HIGH : LOW);
    digitalWrite(SHCP_PIN, HIGH);
  }
  digitalWrite(STCP_PIN, HIGH);
}

// ---------- STATE PERSISTENCE ----------
void saveState() {
  for (int i = 0; i < NUM_RELAYS; i++) EEPROM.write(i, relayState[i] ? 1 : 0);
  EEPROM.commit();
}
void loadState() {
  for (int i = 0; i < NUM_RELAYS; i++) relayState[i] = (EEPROM.read(i) == 1);
}

// ---------- DASHBOARD HTML ----------
const char htmlHeader[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Home Switches</title>
<link rel="manifest" href="/manifest.json">
<style>
 body{font-family:-apple-system,sans-serif;background:#111;color:#eee;text-align:center;padding:20px;margin:0;}
 h1{margin:10px 0 24px;font-size:22px;}
 .grid{display:grid;grid-template-columns:repeat(2,1fr);gap:14px;max-width:520px;margin:auto;}
 .card{background:#1e1e1e;border-radius:14px;padding:16px;}
 .name{font-size:14px;color:#aaa;margin-bottom:10px;min-height:34px;}
 .btn{width:100%;padding:14px;border:none;border-radius:10px;font-size:16px;font-weight:600;cursor:pointer;transition:.15s;}
 .on{background:#2ecc71;color:#062b14;}
 .off{background:#3a3a3a;color:#ccc;}
 #stamp{margin-top:20px;color:#555;font-size:12px;}
</style></head><body>
<h1>🏠 House Switches</h1>
<div class="grid" id="grid">Loading...</div>
<div id="stamp"></div>
<script>
const names = )rawliteral";

const char htmlFooter[] PROGMEM = R"rawliteral(;
function render(states){
  const grid=document.getElementById('grid');
  grid.innerHTML='';
  states.forEach((s,i)=>{
    const div=document.createElement('div');
    div.className='card';
    div.innerHTML='<div class="name">'+names[i]+'</div><button class="btn '+(s?'on':'off')+'" onclick="toggle('+i+')">'+(s?'ON':'OFF')+'</button>';
    grid.appendChild(div);
  });
  document.getElementById('stamp').textContent='Updated '+new Date().toLocaleTimeString();
}
function toggle(i){ fetch('/toggle?relay='+i).then(r=>r.json()).then(render); }
function refresh(){ fetch('/status').then(r=>r.json()).then(render); }
refresh();
setInterval(refresh,4000);
</script></body></html>
)rawliteral";

void handleRoot() {
  String namesJson = "[";
  for (int i = 0; i < NUM_RELAYS; i++) {
    namesJson += "\"" + String(relayNames[i]) + "\"";
    if (i < NUM_RELAYS - 1) namesJson += ",";
  }
  namesJson += "]";

  server.sendHeader("Cache-Control", "no-cache");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  server.sendContent_P(htmlHeader);
  server.sendContent(namesJson);
  server.sendContent_P(htmlFooter);
}

void handleManifest() {
  String json = "{\"name\":\"Home Switches\",\"short_name\":\"Switches\","
                "\"start_url\":\"/\",\"display\":\"standalone\","
                "\"background_color\":\"#111111\",\"theme_color\":\"#111111\"}";
  server.send(200, "application/json", json);
}

void handleStatus() {
  String json = "[";
  for (int i = 0; i < NUM_RELAYS; i++) {
    json += relayState[i] ? "true" : "false";
    if (i < NUM_RELAYS - 1) json += ",";
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handleToggle() {
  if (server.hasArg("relay")) {
    int r = server.arg("relay").toInt();
    if (r >= 0 && r < NUM_RELAYS) {
      relayState[r] = !relayState[r];
      writeShiftRegisters();
      saveState();

      // ---- Serial log for checking purposes ----
      Serial.print("[");
      Serial.print(millis() / 1000);
      Serial.print("s] ");
      Serial.print(relayNames[r]);
      Serial.print(" (Relay ");
      Serial.print(r + 1);
      Serial.print(") turned ");
      Serial.println(relayState[r] ? "ON" : "OFF");
    }
  }
  handleStatus();
}

void setup() {
  Serial.begin(115200);
  pinMode(DS_PIN, OUTPUT);
  pinMode(SHCP_PIN, OUTPUT);
  pinMode(STCP_PIN, OUTPUT);

  EEPROM.begin(NUM_RELAYS);
  loadState();
  writeShiftRegisters();

  WiFi.mode(WIFI_STA);
  WiFi.hostname(HOSTNAME);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin(HOSTNAME)) {
    Serial.println("mDNS responder started: http://homeswitches.local");
    MDNS.addService("http", "tcp", 80);
  }

  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/toggle", handleToggle);
  server.on("/manifest.json", handleManifest);
  server.begin();
}

void loop() {
  server.handleClient();
  MDNS.update();
}
