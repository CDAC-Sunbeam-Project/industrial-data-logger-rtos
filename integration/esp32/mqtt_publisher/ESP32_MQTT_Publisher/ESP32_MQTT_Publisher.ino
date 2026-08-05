#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

/******************** WiFi ************************/
const char* ssid = "vivo T3 5G";
const char* password = "1223334444";

/******************** MQTT ************************/
const char* mqtt_server = "172.18.169.191";
const int mqtt_port = 1883;
const char* mqtt_topic = "industrial/serverroom";
const char* client_id = "ESP32_ServerRoom";

/******************** UART ************************/
HardwareSerial STMSerial(2);

WiFiClient espClient;
PubSubClient client(espClient);

/******************** Sensor Variables ************************/

float temperature = 0;
float humidity = 0;
float pressure = 0;
float gas = 0;

float voltage = 0;
float current = 0;
float power = 0;

int zoneOccupied = 0;

unsigned long eventCount = 0;
unsigned long zoneEntryTime = 0;
unsigned long zoneDurationMin = 0;

/**************************************************/

void connectWiFi()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    Serial.print("Connecting WiFi");

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("WiFi Connected");
    Serial.print("IP : ");
    Serial.println(WiFi.localIP());
}

/**************************************************/

void reconnectMQTT()
{
    while (!client.connected())
    {
        Serial.print("Connecting MQTT...");

        if (client.connect(client_id))
        {
            Serial.println("Connected");
        }
        else
        {
            Serial.print("Failed rc=");
            Serial.println(client.state());
            delay(2000);
        }
    }
}

/**************************************************/

void publishData()
{
    StaticJsonDocument<512> doc;

    doc["temperature"] = temperature;
    doc["humidity"] = humidity;
    doc["pressure"] = pressure;

    doc["gas_ppm"] = gas;

    doc["voltage"] = voltage;
    doc["current"] = current;
    doc["power"] = power;

    doc["zone_occupied"] = zoneOccupied;
    doc["event_count"] = eventCount;
    doc["zone_entry_time_ms"] = zoneEntryTime;
    doc["zone_duration_min"] = zoneDurationMin;

    char payload[512];

    serializeJson(doc, payload);

    client.publish(mqtt_topic, payload);

    Serial.println();
    Serial.println("========== MQTT Published ==========");

    serializeJsonPretty(doc, Serial);

    Serial.println();
}

/**************************************************/

void readSTM32Data()
{
    if (!STMSerial.available())
        return;

    String line = STMSerial.readStringUntil('\n');
    line.trim();

    if (line.length() == 0)
        return;

    // Ignore debug messages
    if (!(isdigit(line[0]) || line[0] == '-'))
        return;

    Serial.println("-----------------------------------");
    Serial.print("Received : ");
    Serial.println(line);

    int result = sscanf(
        line.c_str(),
        "%f,%f,%f,%f,%f,%f,%f,%d,%lu,%lu,%lu",
        &temperature,
        &humidity,
        &pressure,
        &gas,
        &voltage,
        &current,
        &power,
        &zoneOccupied,
        &eventCount,
        &zoneEntryTime,
        &zoneDurationMin);

    if (result == 11)
    {
        Serial.println("Parsing Successful\n");

        Serial.print("Temperature      : ");
        Serial.println(temperature);

        Serial.print("Humidity         : ");
        Serial.println(humidity);

        Serial.print("Pressure         : ");
        Serial.println(pressure);

        Serial.print("Gas (PPM)        : ");
        Serial.println(gas);

        Serial.print("Voltage          : ");
        Serial.println(voltage);

        Serial.print("Current          : ");
        Serial.println(current);

        Serial.print("Power            : ");
        Serial.println(power);

        Serial.print("Zone Occupied    : ");
        Serial.println(zoneOccupied);

        Serial.print("Event Count      : ");
        Serial.println(eventCount);

        Serial.print("Zone Entry Time  : ");
        Serial.println(zoneEntryTime);

        Serial.print("Zone Duration(min): ");
        Serial.println(zoneDurationMin);

        publishData();
    }
    else
    {
        Serial.print("Parsing Failed. Fields Received = ");
        Serial.println(result);

        Serial.print("Received String: ");
        Serial.println(line);
    }
}

/**************************************************/

void setup()
{
    Serial.begin(115200);

    STMSerial.begin(
        115200,
        SERIAL_8N1,
        16,
        17);

    connectWiFi();

    client.setServer(mqtt_server, mqtt_port);
}

/**************************************************/

void loop()
{
    if (WiFi.status() != WL_CONNECTED)
        connectWiFi();

    if (!client.connected())
        reconnectMQTT();

    client.loop();

    readSTM32Data();
}