#include "Meshtastic.h"
#include <IRCClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <FS.h>

// Configuration
const char* configFileName = "/config.json";
const unsigned long pingInterval = 120000; // 2 minutes
const unsigned long ircSendInterval = 1000; // 1 second
const int maxWiFiRetries = 5;
const int maxIRCRetries = 3;

// Configuration Variables
String ssid;
String password;
String ircServer;
int ircPort;
String ircNickname;
String ircUsername;
String ircRealname;
String ircChannel;
String meshtasticFormat = "[Meshtastic] %s";

WiFiClient wifiClient;
IRCClient irc(wifiClient);

std::queue<String> meshtasticQueue;
std::queue<String> ircQueue;

unsigned long lastPing = 0;
unsigned long lastIRCSend = 0;

// LED Status
const int ledPin = LED_BUILTIN; // Change to your LED pin
enum ConnectionStatus { DISCONNECTED, CONNECTING, CONNECTED };
ConnectionStatus wifiStatus = DISCONNECTED;
ConnectionStatus ircStatus = DISCONNECTED;

// Function Prototypes
bool loadConfig();
bool saveConfig();
bool connectWiFi();
bool connectIRC();
void handleIRC();
void handleMeshtastic();
void sendIRCMessage(const String& message);
void sendMeshtasticMessage(const String& message);
void pingIRC();
void updateLEDStatus();

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, HIGH); // Turn LED off

    mesh.init();
    mesh.setDebugOutputStream(&Serial);
    mesh.setNodeInfo("IRC Bridge", 0);

    if (!SPIFFS.begin()) {
        Serial.println("Failed to mount SPIFFS");
        digitalWrite(ledPin, LOW); // Turn LED on
        while (1) delay(1000); // Halt
    }

    if (!loadConfig()) {
        Serial.println("Config load failed. Creating default.");
        saveConfig();
    }

    if (!connectWiFi()) {
        Serial.println("WiFi connection failed.");
    }

    if (!connectIRC()) {
        Serial.println("IRC connection failed.");
    }

    lastPing = millis();
}

void loop() {
    handleMeshtastic();
    handleIRC();
    pingIRC();
    updateLEDStatus();
    delay(100);
}

bool loadConfig() {
    File configFile = SPIFFS.open(configFileName, "r");
    if (!configFile) {
        Serial.println("Failed to open config file. Creating default.");
        return false;
    }

    StaticJsonDocument doc;
    DeserializationError error = deserializeJson(doc, configFile);
    if (error) {
        Serial.print("Failed to parse config file: ");
        Serial.println(error.c_str());
        configFile.close();
        return false;
    }

    if (!doc.containsKey("ssid") || !doc.containsKey("password") ||
        !doc.containsKey("ircServer") || !doc.containsKey("ircPort") ||
        !doc.containsKey("ircNickname") || !doc.containsKey("ircUsername") ||
        !doc.containsKey("ircRealname") || !doc.containsKey("ircChannel")) {
        Serial.println("Config file missing required fields.");
        configFile.close();
        return false;
    }

    ssid = doc["ssid"].as<String>();
    password = doc["password"].as<String>();
    ircServer = doc["ircServer"].as<String>();
    ircPort = doc["ircPort"].as<int>();
    ircNickname = doc["ircNickname"].as<String>();
    ircUsername = doc["ircUsername"].as<String>();
    ircRealname = doc["ircRealname"].as<String>();
    ircChannel = doc["ircChannel"].as<String>();
    if(doc.containsKey("meshtasticFormat")){
        meshtasticFormat = doc["meshtasticFormat"].as<String>();
    }

    configFile.close();
    return true;
}

bool saveConfig() {
    StaticJsonDocument doc;
    doc["ssid"] = "YOUR_WIFI_SSID";
    doc["password"] = "YOUR_WIFI_PASSWORD";
    doc["ircServer"] = "irc.akita.chat";
    doc["ircPort"] = 6667;
    doc["ircNickname"] = "MeshtasticBot";
    doc["ircUsername"] = "MeshtasticBot";
    doc["ircRealname"] = "Meshtastic IRC Bridge";
    doc["ircChannel"] = "#meshtastic";
    doc["meshtasticFormat"] = "[Meshtastic] %s";

    File configFile = SPIFFS.open(configFileName, "w");
    if (!configFile) {
        Serial.println("Failed to open config file for writing");
        return false;
    }
    serializeJson(doc, configFile);
    configFile.close();
    return true;
}

bool connectWiFi() {
    wifiStatus = CONNECTING;
    int retries = 0;
    WiFi.begin(ssid.c_str(), password.c_str());

    while (WiFi.status() != WL_CONNECTED && retries < maxWiFiRetries) {
        delay(1000);
        Serial.print("Connecting to WiFi... ");
        Serial.println(retries + 1);
        retries++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        wifiStatus = CONNECTED;
        return true;
    } else {
        Serial.println("WiFi connection failed.");
        wifiStatus = DISCONNECTED;
        return false;
    }
}

bool connectIRC() {
    ircStatus = CONNECTING;
    int retries = 0;
    while (!irc.connect(ircServer.c_str(), ircPort, ircNickname.c_str(), ircUsername.c_str(), ircRealname.c_str()) && retries < maxIRCRetries) {
        delay(5000);
        Serial.print("Connecting to IRC... ");
        Serial.println(retries + 1);
        retries++;
    }

    if (irc.connected()) {
        Serial.println("Connected to IRC");
        irc.join(ircChannel.c_str());
        ircStatus = CONNECTED;
        return true;
    } else {
        Serial.println("Connection to IRC failed.");
        ircStatus = DISCONNECTED;
        return false;
    }
}

void handleIRC() {
irc.loop();
    if (irc.available()) {
        String message = irc.read();
        if (message.indexOf("PRIVMSG " + ircChannel + " :") != -1) {
            int start = message.indexOf("PRIVMSG " + ircChannel + " :") + String("PRIVMSG " + ircChannel + " :").length();
            String ircMsg = message.substring(start);
            ircQueue.push(ircMsg);
        }
    }

    if (!ircQueue.empty() && millis() - lastIRCSend >= ircSendInterval) {
        sendMeshtasticMessage("[IRC] " + ircQueue.front());
        ircQueue.pop();
        lastIRCSend = millis();
    }

    if (ircStatus == DISCONNECTED) {
        if (connectIRC()) {
            Serial.println("IRC reconnected.");
        }
    }
}

void handleMeshtastic() {
    if (mesh.available()) {
        ReceivedPacket packet = mesh.receive();
        if (packet.decoded.payload.length() > 0) {
            String meshtasticMsg = packet.decoded.payload.toString();
            meshtasticQueue.push(meshtasticMsg);
        }
    }

    if (!meshtasticQueue.empty()) {
        String msg = meshtasticFormat;
        msg.replace("%s", meshtasticQueue.front());
        sendIRCMessage(msg);
        meshtasticQueue.pop();
    }

    if (wifiStatus == DISCONNECTED) {
        if (connectWiFi()) {
            Serial.println("WiFi reconnected.");
            connectIRC();
        }
    }
}

void sendIRCMessage(const String& message) {
    if (millis() - lastIRCSend >= ircSendInterval && ircStatus == CONNECTED) {
        irc.send(ircChannel.c_str(), message.c_str());
        lastIRCSend = millis();
    }
}

void sendMeshtasticMessage(const String& message) {
    mesh.sendText(message);
}

void pingIRC() {
    if (millis() - lastPing >= pingInterval && ircStatus == CONNECTED) {
        irc.sendRaw("PING " + ircServer);
        lastPing = millis();
    }
}

void updateLEDStatus() {
    if (wifiStatus == CONNECTED && ircStatus == CONNECTED) {
        digitalWrite(ledPin, HIGH); // LED off
    } else {
        digitalWrite(ledPin, LOW); // LED on
    }
}
