#include "DomoticzMQTT.h"

DomoticzMQTT::DomoticzMQTT(IPAddress mqttAddress, uint mqttPort)
{
    this->mqttAddress = mqttAddress;
    this->mqttPort = mqttPort;

    this->mqttClient = new AsyncMqttClient();
    this->mqttReconnectTimer = new Ticker();
    this->wifiReconnectTimer = new Ticker();
}

void DomoticzMQTT::init()
{
    this->wifiConnectHandler = WiFi.onStationModeGotIP([=] (const WiFiEventStationModeGotIP& event) {return this->onWifiConnect(event);});
    this->wifiDisconnectHandler = WiFi.onStationModeDisconnected([=] (const WiFiEventStationModeDisconnected& event) {return this->onWifiDisconnect(event);});


    this->mqttClient->onConnect([=] (bool sessionPresent) {
        this->onMqttConnect(sessionPresent); 
    });
    this->mqttClient->onDisconnect([=] (AsyncMqttClientDisconnectReason reason) {
        this->onMqttDisconnect(reason);
    });
    this->mqttClient->onSubscribe([=] (uint16_t packetId, uint8_t qos) {
        this->onMqttSubscribe(packetId, qos);
    });
    this->mqttClient->onUnsubscribe([=] (uint16_t packetId) {
        this->onMqttUnsubscribe(packetId);
    });
    this->mqttClient->onMessage([=] (char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
        this->onMqttMessage(topic, payload, properties, len, index, total);
    });
    this->mqttClient->onPublish([=] (uint16_t packetId) {
        this->onMqttPublish(packetId);
    });
    this->mqttClient->setServer(this->mqttAddress, this->mqttPort);

}


void DomoticzMQTT::onMqttConnect(bool sessionPresent)
{
    #ifdef DOMOTICZ_MQTT_DEBUG
    Serial.println("Connected to MQTT.");
    Serial.print("Session present: ");
    Serial.println(sessionPresent);

    Serial.print("Subscribing at QoS 2, packetId: ");
    Serial.println(packetIdSub);
    #endif
    uint16_t packetIdSub = this->mqttClient->subscribe("domoticz/out", 2);
    this->sendConnectedEvent();
}


void DomoticzMQTT::onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
    #ifdef DOMOTICZ_MQTT_DEBUG
    Serial.println("Publish received.");
    #endif
    // Serial.print("  topic: ");
    // Serial.println(topic);
    // Serial.print("  qos: ");
    // Serial.println(properties.qos);
    // Serial.print("  dup: ");
    // Serial.println(properties.dup);
    // Serial.print("  retain: ");
    // Serial.println(properties.retain);
    // Serial.print("  len: ");
    // Serial.println(len);
    // Serial.print("  index: ");
    // Serial.println(index);
    // Serial.print("  total: ");
    // Serial.println(total);


    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        #ifdef DOMOTICZ_MQTT_DEBUG
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        #endif
        return;
    }
    
    int idx = doc["idx"].as<int>();
    if(idx > 0) {
        DomoticzDevice data = {};
        data.idx = idx;
        data.svalue1 = doc["svalue1"].as<float>();
        data.svalue2 = doc["svalue2"].as<float>();
        data.svalue3 = doc["svalue3"].as<float>();
        data.svalue4 = doc["svalue4"].as<float>();
        data.nvalue = doc["nvalue"].as<int>();
        data.Battery = doc["Battery"].as<int>();
        data.RSSI = doc["RSSI"].as<int>();
        // data.dtype = doc["dtype"].as<String>();

        this->sendDeviceEvent(idx, data);
    }
}



void DomoticzMQTT::sendTemperature(int idx, float temp)
{
    String data = String(temp,2);
    this->updateDevice(idx, data);
}

void DomoticzMQTT::sendTemperatureAndHumidity(int idx, float temp, float hum)
{
    String data = String(temp,2) + ";" + String(hum,2) + ";0";
    this->updateDevice(idx, data);
}

void DomoticzMQTT::updateDevice(int idx, String value)
{
    StaticJsonDocument<200> doc;
    doc["idx"] = idx;
    doc["nvalue"] = 0;
    doc["svalue"] = value;

    this->sendJson(doc);
}

void DomoticzMQTT::requestDeviceData(int idx)
{
    StaticJsonDocument<200> doc;
    doc["idx"] = idx;
    doc["command"] = "getdeviceinfo";

    this->sendJson(doc);    
}
void DomoticzMQTT::sendJson(JsonDocument json)
{
    char output[200];

    serializeJson(json, output);

    #ifdef DOMOTICZ_MQTT_DEBUG
    Serial.print("Send json:");
    Serial.println(output);
    #endif

    this->mqttClient->publish("domoticz/in", 0, true, output);
}