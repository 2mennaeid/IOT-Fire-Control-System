#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h> 
const char* ssid = "Wokwi-GUEST";
const char* password = "";
#define BETA  3950
unsigned long previousMillis = 0;
const long interval = 1000;
const char* mqttServer = "test.mosquitto.org";  // Replace with your MQTT broker address
const int mqttPort = 1883;  // MQTT broker port (typically 1883)
const char* mqttTopic = "Temperature";  // The MQTT topic you want to publish to
const char* mqttTopic1 = "Gas";
WiFiClient espClient;
PubSubClient client(espClient);

#define Buzzer_PIN    2
#define GASPin         34
#define RL_VALUE     10.0 // Resistance of the load resistor (in kilohms)
#define RO_CLEAN_AIR  9.83  // Resistance of the sensor in clean air

void reconnect() 
{
  while (!client.connected()) 
  {
    Serial.println("Attempting MQTT connection...");
    if (client.connect("ArduinoClient")) 
    {
      Serial.println("Connected to MQTT broker");
    }
    else 
    {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Retry in 5 seconds");
      delay(5000);
    }
  }
}

float readMQResistance() {
  int sensorValue = analogRead(GASPin);
  float voltage = sensorValue * (5.0 / 1023.0);  // Convert analog value to voltage
  float RS = ((5.0 - voltage) / voltage) * RL_VALUE;  // Calculate sensor resistance
  return RS;
}

float getMQPercentage() {
  float RS_RO_RATIO = readMQResistance() / RO_CLEAN_AIR;
  // Use the ratio to calculate the gas concentration
  float ppm_log = (log10(RS_RO_RATIO) - 1.028) / -0.3996;
  float ppm = pow(10, ppm_log);
  return ppm;
}

void setup() 
{ 
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  client.setServer(mqttServer, mqttPort);
  reconnect();
  analogReadResolution(10);
  pinMode(32,INPUT);
  pinMode(15, OUTPUT);
  pinMode(34, INPUT);
}

void loop()
{
  
  int TempValue = analogRead(32);
  float gasValue = getMQPercentage();
  float celsius = 1 / (log(1 / (1023. / TempValue - 1)) / BETA + 1.0 / 298.15) - 273.15;
  String temperatureStr = String(celsius, 2); // 2 decimal places
  String GasStr = String(gasValue, 2); // 2 decimal places
  

  // Publish temperature data to MQTT topic
  if (client.publish(mqttTopic, temperatureStr.c_str()) ) 
  {
    Serial.println("Published to MQTT topic for temperature : " + temperatureStr);
    if(celsius > 50.0)    //turn on the buzzer if the temperature is > 50
    {
      digitalWrite(Buzzer_PIN, HIGH);
      tone(Buzzer_PIN,500);
    }
    else
    {
      digitalWrite(Buzzer_PIN, LOW);
      tone(Buzzer_PIN,0);
    }
  }
  if (client.publish(mqttTopic1, GasStr.c_str()) ) 
  {
    Serial.println("Published to MQTT topic for gas: " + GasStr);
    
  }

  else 
  {
    Serial.println("Failed to publish to MQTT topic");
  }

    // Allow time for MQTT messages to be sent and received
     client.loop();
    // Publish data every 5 seconds (adjust as needed)
    if (!client.connected()) {
    reconnect();
  }
    
  delay(5000);
}