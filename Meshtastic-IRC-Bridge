#include "Meshtastic.h"
#include <IRCClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <FS.h>

// Configuration File
const char* configFileName = "/config.json";

// Configuration Variables (Loaded from config file)
String ssid;
String password;
String ircServer;
int ircPort;
String ircNickname;
String ircUsername;
String ircRealname;
String ircChannel;

WiFiClient wifiClient;
IRCClient irc(wifiClient);

String receivedMessage = "";
String ircMessage = "";
bool newMessage = false;
bool newIrcMessage = false;

unsigned long lastPing = 0;
const unsigned long pingInterval = 120000; // 2 minutes

// Message Queues
std::queue<String> meshtasticQueue;
std::queue<String> ircQueue;

// Rate Limiting
unsigned long lastIRCSend = 0;
const unsigned long ircSendInterval = 1000; // 1 second

// Function Prototypes
void loadConfig();
void saveConfig();
bool connectWiFi();
bool connectIRC();
void handleIRC();
void handleMeshtastic();
void sendIRCMessage(const String& message);
void sendMeshtasticMessage(const String& message);
void pingIRC();

void setup() {
  Serial.begin(115200);
  delay(1000);

  mesh.init();
  mesh.setDebugOutputStream(&Serial);
  mesh.setNodeInfo("IRC Bridge", 0);

  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount SPIFFS");
    return;
  }

  loadConfig();

  if (!connectWiFi()) {
    Serial.println("WiFi connection failed. Restarting.");
    ESP.restart();
  }

  if (!connectIRC()) {
    Serial.println("IRC connection failed. Restarting.");
    ESP.restart();
  }

  lastPing = millis();
}

void loop() {
  handleMeshtastic();
  handleIRC();
  pingIRC();

  delay(100);
}

void loadConfig() {
  File configFile = SPIFFS.open(configFileName, "r");
  if (!configFile) {
    Serial.println("Failed to open config file. Creating default.");
    ssid = "YOUR_WIFI_SSID";
    password = "YOUR_WIFI_PASSWORD";
    ircServer = "irc.akita.chat";
    ircPort = 6667;
    ircNickname = "MeshtasticBot";
    ircUsername = "MeshtasticBot";
    ircRealname = "Meshtastic IRC Bridge";
    ircChannel = "#meshtastic";
    saveConfig();
    return;
  }

  StaticJsonDocument doc;
  DeserializationError error = deserializeJson(doc, configFile);
  if (error) {
    Serial.println("Failed to parse config file");
    return;
  }

  ssid = doc["ssid"].as<String>();
  password = doc["password"].as<String>();
  ircServer = doc["ircServer"].as<String>();
  ircPort = doc["ircPort"].as<int>();
  ircNickname = doc["ircNickname"].as<String>();
  ircUsername = doc["ircUsername"].as<String>();
  ircRealname = doc["ircRealname"].as<String>();
  ircChannel = doc["ircChannel"].as<String>();
  configFile.close();
}

void saveConfig() {
  StaticJsonDocument doc;
  doc["ssid"] = ssid;
  doc["password"] = password;
  doc["ircServer"] = ircServer;
  doc["ircPort"] = ircPort;
  doc["ircNickname"] = ircNickname;
  doc["ircUsername"] = ircUsername;
  doc["ircRealname"] = ircRealname;
  doc["ircChannel"] = ircChannel;

  File configFile = SPIFFS.open(configFileName, "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return;
  }
  serializeJson(doc, configFile);
  configFile.close();
}

bool connectWiFi() {
  WiFi.begin(ssid.c_str(), password.c_str());
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    return false;
  }
}

bool connectIRC() {
  if (irc.connect(ircServer.c_str(), ircPort, ircNickname.c_str(), ircUsername.c_str(), ircRealname.c_str())) {
    Serial.println("Connected to IRC");
    irc.join(ircChannel.c_str());
    return true;
  } else {
    Serial.println("Connection to IRC failed");
    return false;
  }
}

void handleIRC() {
  irc.loop();
  if (irc.available()) {
    String message = irc.read();
    if (message.indexOf("PRIVMSG " + ircChannel + " :") != -1) {
      int start = message.indexOf("PRIVMSG " + ircChannel + " :") + String("PRIVMSG " + ircChannel + " :").length();
      ircMessage = message.substring(start);
      ircQueue.push(ircMessage);
    }
  }

  if (!ircQueue.empty() && millis() - lastIRCSend >= ircSendInterval) {
    sendMeshtasticMessage("[IRC] " + ircQueue.front());
    ircQueue.pop();
    lastIRCSend = millis();
  }
}

void handleMeshtastic() {
  if (mesh.available()) {
    ReceivedPacket packet = mesh.receive();
    if (packet.decoded.payload.length() > 0) {
      receivedMessage = packet.decoded.payload.toString();
      meshtasticQueue.push(receivedMessage);
    }
  }

  if (!meshtasticQueue.empty()) {
    sendIRCMessage("[Meshtastic] " + meshtasticQueue.front());
    meshtasticQueue.pop();
  }
}

void sendIRCMessage(const String& message) {
  if (millis() - lastIRCSend >= ircSendInterval) {
    irc.send(ircChannel.c_str(), message.c_str());
    lastIRCSend = millis();
  }
}

void sendMeshtasticMessage(const String& message) {
  mesh.sendText(message);
}

void pingIRC() {
  if (millis() - lastPing >= pingInterval) {
    irc.sendRaw("PING " + ircServer);
    lastPing = millis();
  }
}
