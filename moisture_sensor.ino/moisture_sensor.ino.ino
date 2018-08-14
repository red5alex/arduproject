/*
 */

#include <ESP8266WiFi.h>
const char* ssid     = "my_ssid";
const char* password = "my_wpa_key";

#include "ThingSpeak.h"
unsigned long myChannelNumber = 557026;
const char * myWriteAPIKey = "my_thingspeak_apikey";

#define VOLTAGE_MAX 3.3
#define VOLTAGE_MAXCOUNTS 1023.0 

float min_value;
float max_value;

int status = WL_IDLE_STATUS;
WiFiClient  client;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);

  min_value = (double)analogRead(A0);
  max_value = (double)analogRead(A0)+1;

  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, LOW);    // LED on 
    delay(250); 
    digitalWrite(LED_BUILTIN, HIGH);   // LED off 
    delay(250);                      
    Serial.print(".");
  }

  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  ThingSpeak.begin(client);

  Serial.print("value");
  Serial.print("\t");
  Serial.print("min");
  Serial.print("\t");
  Serial.print("max");
  Serial.print("\t");
  Serial.print("V");
  Serial.print("\t");
  Serial.print("relative_moisture");
  Serial.println(" ");
  
}

// the loop routine runs over and over again forever:
void loop() {

  //if WiFi lost connection, flash LED while waiting
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, LOW);    // LED on 
    delay(250); 
    digitalWrite(LED_BUILTIN, HIGH);   // LED off 
    delay(250);                      
    Serial.print(".");
    Serial.print(".");

  // read sensor value, set calibration points (min, max)
  float sensor_value = (double)analogRead(A0);
  if (sensor_value > max_value) max_value = sensor_value;
  if (sensor_value < min_value) min_value = sensor_value;

  // calculate voltage and moisture from sensor data
  float relative_moisture = 100 - (sensor_value - min_value) / (max_value - min_value) * 100;
  float voltage = sensor_value * (VOLTAGE_MAX / VOLTAGE_MAXCOUNTS);

  // write diagnostic data to serial
  Serial.print(sensor_value);
  Serial.print("\t");
  Serial.print(min_value);
  Serial.print("\t");
  Serial.print(max_value);
  Serial.print("\t");
  Serial.print(voltage);
  Serial.print("\t");
  Serial.print(relative_moisture);
  Serial.println(" ");

  // flash LED before sending data
  digitalWrite(LED_BUILTIN, LOW);    // LED on 
  delay(250); 
  digitalWrite(LED_BUILTIN, HIGH);   // LED off 
  delay(250);

  // send data to ThingSpeak.com
  ThingSpeak.setField(1, voltage);
  ThingSpeak.setField(2, relative_moisture);
  ThingSpeak.setField(3, sensor_value);
  ThingSpeak.setField(4, min_value);
  ThingSpeak.setField(5, max_value);
  ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

  // flash LED after sending data
  digitalWrite(LED_BUILTIN, LOW);    // LED on 
  delay(250); 
  digitalWrite(LED_BUILTIN, HIGH);   // LED off 

  // activate LED if moisture below 25%
  if (relative_moisture < 25)
      digitalWrite(LED_BUILTIN, LOW);    // LED on 
  else
    digitalWrite(LED_BUILTIN, HIGH);   // LED off 

  // wait 20 seconds, ThingSpeak will only accept updates every 15 seconds.
  delay(20000); 
  
  }
}
