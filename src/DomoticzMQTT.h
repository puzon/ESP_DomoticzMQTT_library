#include <Arduino.h>
#include <list>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>
#include <ArduinoJson.h>

#define EVENTMANAGER_LISTENER_LIST_SIZE 16

enum DomoticzMQTTEventType {
    Device,
    Connected,
    Disconnected,
};

struct DomoticzDevice {
    int idx;
    float svalue1;
    float svalue2;
    float svalue3;
    float svalue4;

    int nvalue;
    int Battery;
    int RSSI;
};

typedef void ( *DeviceEventListener )( int idx, DomoticzDevice data );
class DomoticzMQTT
{
    
    public:
        DomoticzMQTT(IPAddress mqttAddress, uint mqttPort = 1883);

        void init();

        void addDeviceEventListener(int idx, DeviceEventListener listener)
        {
            this->addEventListener(idx, Device, listener);
        }
        
        void addConnectedEventListener(DeviceEventListener listener)
        {
            this->addEventListener(0, Connected, listener);
        }

        void addDisconnectedEventListener(DeviceEventListener listener)
        {
            this->addEventListener(0, Disconnected, listener);
        }

        void addEventListener(int idx, DomoticzMQTTEventType type, DeviceEventListener listener)
        {
            this->mListeners[ this->mNumListeners ].callback = listener;
            this->mListeners[ this->mNumListeners ].idx = idx;
            this->mListeners[ this->mNumListeners ].type = type;
            this->mNumListeners++;

        }
    

        void sendDeviceData(int idx, String svalue1);

        void sendTemperature(int idx, float temp);
        void sendTemperatureAndHumidity(int idx, float temp, float hum);
        void updateDevice(int idx, String value);
        void requestDeviceData(int idx);

    private:
        static const int kMaxListeners = EVENTMANAGER_LISTENER_LIST_SIZE;
        // Listener structure and corresponding array
        struct ListenerItem
        {
            DeviceEventListener	callback;		// The listener function
            int				idx;		// The event code
            DomoticzMQTTEventType type;
        };
        ListenerItem mListeners[ kMaxListeners ];
        int mNumListeners = 0;
        // std::list<DomoticzEventHandler> sCbEventList;

        IPAddress mqttAddress;
        int mqttPort;

        WiFiClient          *client;
        AsyncMqttClient     *mqttClient;
        Ticker              *mqttReconnectTimer;

        WiFiEventHandler    wifiConnectHandler;
        WiFiEventHandler    wifiDisconnectHandler;
        Ticker              *wifiReconnectTimer;


        void sendJson(JsonDocument doc);

        void onWifiConnect(const WiFiEventStationModeGotIP& event) {
            #ifdef DOMOTICZ_MQTT_DEBUG
            Serial.println("DomoticzMQTT: Connected to Wi-Fi.");
            #endif
            this->connectToMqtt();
        }

        void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
            #ifdef DOMOTICZ_MQTT_DEBUG
            Serial.println("DomoticzMQTT: Disconnected from Wi-Fi.");
            #endif
            this->mqttReconnectTimer->detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
            // wifiReconnectTimer.once(2, connectToWifi);
        }


        void connectToMqtt() {
            #ifdef DOMOTICZ_MQTT_DEBUG
            Serial.println("Connecting to MQTT...");
            #endif
            this->mqttClient->connect();
        }
        void onMqttConnect(bool sessionPresent);
        
        void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
            #ifdef DOMOTICZ_MQTT_DEBUG
            Serial.println("Disconnected from MQTT.");
            #endif
            this->sendDisconnectedEvent();
            if (WiFi.isConnected()) {
                this->mqttReconnectTimer->once(2, [=] { this->connectToMqtt(); });
            }
        }

        void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
            #ifdef DOMOTICZ_MQTT_DEBUG
            Serial.println("Subscribe acknowledged.");
            Serial.print("  packetId: ");
            Serial.println(packetId);
            Serial.print("  qos: ");
            Serial.println(qos);
            #endif
        }

        void onMqttUnsubscribe(uint16_t packetId) {
            #ifdef DOMOTICZ_MQTT_DEBUG
            Serial.println("Unsubscribe acknowledged.");
            Serial.print("  packetId: ");
            Serial.println(packetId);
            #endif
        }


        void onMqttPublish(uint16_t packetId) {
            #ifdef DOMOTICZ_MQTT_DEBUG
            Serial.println("Publish acknowledged.");
            Serial.print("  packetId: ");
            Serial.println(packetId);
            #endif
        }

        void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);

        int sendDeviceEvent( int idx, DomoticzDevice data )
        {
            int handlerCount = 0;
            for ( int i = 0; i < mNumListeners; i++ )
            {
                if ( ( mListeners[ i ].callback != 0 ) && ( mListeners[ i ].idx != 0 && mListeners[ i ].idx == idx ) && ( mListeners[ i ].type == Device ))
                {
                    handlerCount++;
                    (*mListeners[ i ].callback)( idx, data );
                }
            }
            return handlerCount;
        }
        int sendConnectionEvent(DomoticzMQTTEventType type)
        {
            int handlerCount = 0;
            for ( int i = 0; i < mNumListeners; i++ )
            {
                if ( ( mListeners[ i ].callback != 0 ) && ( mListeners[ i ].type == type ))
                {
                    handlerCount++;
                    DomoticzDevice data = {};
                    (*mListeners[ i ].callback)(0,data);
                }
            }
            return handlerCount;
        }
        int sendConnectedEvent()
        {
            return this->sendConnectionEvent(Connected);
        }
        int sendDisconnectedEvent()
        {
            return this->sendConnectionEvent(Disconnected);
        }


        
};