# ESP DomoticzMQTT library
Arduino Library for ESP8266 to communicate with Domoticz MQTT service

# Dependencies
- static esp WIFI class
- AsyncMqttClient
- ArduinoJson

# Example
```c++
#include <Arduino.h>

#define MQTT_HOST IPAddress(10, 20, 3, 62)
#define MQTT_PORT 1883

#define WIFI_SSID  "SSID"
#define WIFI_PASSWORD  "PASS"

#define DOMOTICZ_TEMPERATURE_IDX 1
#define DOMOTICZ_TEMPERATURE_SETPOINT_IDX 2
#define DOMOTICZ_BOILER_SWITCH_IDX 3

#define BOILER_PIN BUILTIN_LED

DomoticzMQTT domoticzClient(MQTT_HOST);

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void setup()
{
    pinMode(BOILER_PIN, OUTPUT);
    digitalWrite(BOILER_PIN, HIGH);
    domoticzClient.init();
    // Domoticz sent measured temperature
    domoticzClient.addDeviceEventListener(DOMOTICZ_TEMPERATURE_IDX, [=](int idx, DomoticzDevice data) {
        Serial.print("Temperature changed:")
        Serial.print(data.svalue);
        Serial.println("°C.");
    });
    // Domoticz turned on boiler
    domoticzClient.addDeviceEventListener(DOMOTICZ_BOILER_SWITCH_IDX, [=](int idx, DomoticzDevice data) {
        Serial.print("Turn boiler ");
        if(data.nvalue == 1) {
            Serial.println("on.");
        } else {
            Serial.println("off.");
        }
        digitalWrite(BOILER_PIN, data.nvalue == 1 ? 0 : 1);
    });
  
    // Request devices state when connected to MQTT network
    domoticzClient.addConnectedEventListener( [=] (int idx, DomoticzDevice data) {
        domoticzClient.requestDeviceData(DOMOTICZ_TEMPERATURE_IDX);
        domoticzClient.requestDeviceData(DOMOTICZ_BOILER_IDX);
    });
    
    connectToWifi();
}

void loop() {
    float newTemp = 19.0f;
    // Send setpoint temperature to Domoticz
    domoticzClient.sendTemperature(DOMOTICZ_TEMPERATURE_SETPOINT_IDX, newTemp);
    delay(5000);
}
```