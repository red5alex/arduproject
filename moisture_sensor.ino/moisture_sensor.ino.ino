/*
   upload settings for D1 board:
   80 MHz
   1M SPIFF
   57600 baud
*/

#include <I2CSoilMoistureSensor.h>
#include <Wire.h>

I2CSoilMoistureSensor sensor;

#include <ESP8266WiFi.h>
const char* ssid     = "my_ssid";
const char* password = "my_wpa_key";

#include "ThingSpeak.h"
unsigned long myChannelNumber = 557026;
const char * myWriteAPIKey = "my_thingspeak_apikey";

#define VOLTAGE_MAX 3.3
#define VOLTAGE_MAXCOUNTS 1023.0

float min_value;  // analog voltage
float max_value;  // analog voltage

float capacitance;
float temperature;

int status = WL_IDLE_STATUS;
WiFiClient  client;


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);

  // initialize I2C bus and digital sensor
  Wire.begin();
  sensor.begin(); // reset sensor
  delay(1000); // give some time to boot up

  // write diagnostic information on sensor
  Serial.print("I2C Soil Moisture Sensor Address: ");
  Serial.println(sensor.getAddress(), HEX);
  Serial.print("Sensor Firmware version: ");
  Serial.println(sensor.getVersion(), HEX);
  Serial.println();

  // initialize calibration value
  min_value = (double)analogRead(A0);
  max_value = (double)analogRead(A0) + 1;

  //connect to wifi
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

  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("");
  Serial.print("WiFi connected: ");
  Serial.println(WiFi.localIP());

  // initialze thingspeak
  ThingSpeak.begin(client);

  // header for diagnostic output
  Serial.print("value");
  Serial.print("\t");
  Serial.print("min");
  Serial.print("\t");
  Serial.print("max");
  Serial.print("\t");
  Serial.print("V");
  Serial.print("\t");
  Serial.print("WC");
  Serial.print("\t");
  Serial.print("Cap*");
  Serial.print("\t");
  Serial.print("Temp*");
  Serial.println(" ");
}


void loop() {

  //Check if WiFi is down, wait for reconnect and flash LED if so
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, LOW);    // LED on
    delay(250);
    digitalWrite(LED_BUILTIN, HIGH);   // LED off
    delay(250);
    Serial.print(".");
  }

  // read analog sensor value, set calibration points (min, max)
  float sensor_value = (double)analogRead(A0);
  if (sensor_value > max_value) max_value = sensor_value;
  if (sensor_value < min_value) min_value = sensor_value;

  // calculate voltage and moisture from analog sensor data
  float relative_moisture = 100 - (sensor_value - min_value) / (max_value - min_value) * 100;
  float voltage = sensor_value * (VOLTAGE_MAX / VOLTAGE_MAXCOUNTS);

  // read digital sensor value
  while (sensor.isBusy()) delay(50); // available since FW 2.3
  capacitance = sensor.getCapacitance();
  temperature = sensor.getTemperature() / (float)10;
  //TODO: light = sensor.getLight(true)
  //TODO: sensor.sleep(); // available since FW 2.3

  //restart i2c bus is down
  while (capacitance == 65535) {

    //reset i2c bus and flash while giving time to boot up
    Serial.println("sensor down, trying to restart i2c...");
    Wire.begin();
    sensor.begin(); // reset sensor
    for (int i = 0; i < 20; i++) {
      digitalWrite(LED_BUILTIN, LOW);    // LED on
      delay(50);
      digitalWrite(LED_BUILTIN, HIGH);   // LED off
      delay(50);
    }
    // read new measurement
    capacitance = sensor.getCapacitance();
    temperature = sensor.getTemperature() / (float)10;
  }

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
  Serial.print("\t");
  Serial.print(capacitance);
  Serial.print("\t");
  Serial.print(temperature);
  Serial.println(" ");

  // send data to ThingSpeak.com
  digitalWrite(LED_BUILTIN, LOW);    // LED on
  delay(125);
  digitalWrite(LED_BUILTIN, HIGH);   // LED off
  
  ThingSpeak.setField(1, voltage);
  ThingSpeak.setField(2, relative_moisture);
  ThingSpeak.setField(3, sensor_value);
  ThingSpeak.setField(4, min_value);
  ThingSpeak.setField(5, max_value);
  ThingSpeak.setField(6, capacitance);
  ThingSpeak.setField(8, temperature);
  ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

  digitalWrite(LED_BUILTIN, LOW);    // LED on
  delay(125);
  digitalWrite(LED_BUILTIN, HIGH);   // LED off

  // wait 20 seconds, ThingSpeak will only accept updates every 15 seconds.
  delay(20000);
}
