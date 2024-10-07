#include <WiFi.h>
#include "max6675.h"
#include <TinyGPS++.h>

// Thermocouple pin definitions
int thermoDO = 19;
int thermoCS = 2;
int thermoCLK = 18;

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

// Soil moisture sensor pin definition
const int sensor_pin = 34;
int sensor_analog;

// LDR pin definition
const int ldrPin = 35;  // Analog pin connected to the LDR

// GPS module pins
const int gpsRX = 16;
const int gpsTX = 17;

// Replace with your network credentials
const char* ssid = "ESP32-TempControl";
const char* password = "Temp1234";

// Set web server port number to 80
WiFiServer server(80);

String header; // Store the HTTP request

TinyGPSPlus gps; // Create an instance of the TinyGPS++ object

// Memory to store the last 10 readings
struct SensorData {
  float temperature;
  int moisture;
  int lightIntensity;
  double latitude;
  double longitude;
  unsigned long timestamp;
};

SensorData readings[10];
int readingCount = 0;
int currentIndex = 0;

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  // Initialize serial communication for GPS module
  Serial2.begin(115200, SERIAL_8N1, gpsRX, gpsTX);

  // Initial message for thermocouple
  Serial.println("MAX6675 test");

  // Initial message for LDR
  pinMode(ldrPin, INPUT); 

  // Initialize the ESP32 as an access point
  WiFi.softAP(ssid, password);

  // Print the IP address
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Start the server
  server.begin();

  // Wait for MAX6675 to stabilize
  delay(2000);
}

float realTimeTemperature;
int realTimeMoisture;
int realTimeLightIntensity;
float realTimeLatitude;
float realTimeLongitude;

void loop() {
  // Read temperature from thermocouple
  realTimeTemperature = thermocouple.readCelsius();
  Serial.print("C = ");
  Serial.print(realTimeTemperature);
  Serial.println("°C");

  // Read soil moisture sensor value
  sensor_analog = analogRead(sensor_pin);
  realTimeMoisture = (int)(100 - ((sensor_analog / 4095.00) * 100));
  Serial.print("Moisture = ");
  Serial.print(realTimeMoisture);
  Serial.println("%");

  // Read light intensity from LDR
  int sensorValue = analogRead(ldrPin);
  // Adjust these values based on your observations of min and max sensor values
  int minSensorValue = 39;  // Set this to the minimum observed value from the LDR
  int maxSensorValue = 4095;  // Set this to the maximum observed value from the LDR
  realTimeLightIntensity = map(sensorValue, minSensorValue, maxSensorValue, 0, 100);  // Corrected range for 12-bit ADC
  //realTimeLightIntensity = constrain(realTimeLightIntensity, 0, 100); // Ensure the value stays within 0 to 100
  Serial.print("Light Intensity: ");
  Serial.print(realTimeLightIntensity);
  Serial.println("%");

  // Read GPS data
  while (Serial2.available() > 0) {
    gps.encode(Serial2.read());
  }

  realTimeLatitude = 0.0;
  realTimeLongitude = 0.0;

  // Print GPS data to Serial monitor if available
  if (gps.location.isUpdated()) {
    realTimeLatitude = gps.location.lat();
    realTimeLongitude = gps.location.lng();
    Serial.print("Latitude = ");
    Serial.println(realTimeLatitude);
    Serial.print("Longitude = ");
    Serial.println(realTimeLongitude);
  } else {
    Serial.println("Waiting for GPS data...");
  }

  // Delay to ensure the MAX6675 updates properly and give time for serial output to be readable
  delay(2000);

  WiFiClient client = server.available();

  if (client) {
    Serial.println("New Client.");
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        header += c;
        if (c == '\n') {
          if (currentLine.length() == 0) {
            if (header.indexOf("GET /measure") >= 0) {
              storeCurrentData(realTimeTemperature, realTimeMoisture, realTimeLightIntensity, realTimeLatitude, realTimeLongitude);
              sendHtmlResponse(client, "Data stored successfully!");
            } else if (header.indexOf("GET /data") >= 0) {
              sendDataJsonResponse(client);
            } else if (header.indexOf("GET /table") >= 0) {
              sendTableResponse(client);
            } else if (header.indexOf("GET /") >= 0) {
              sendHtmlResponse(client, "");
            }
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    header = "";
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

void storeCurrentData(float temperature, int moisture, int lightIntensity, double latitude, double longitude) {
  readings[currentIndex].temperature = temperature;
  readings[currentIndex].moisture = moisture;
  readings[currentIndex].lightIntensity = lightIntensity;
  readings[currentIndex].latitude = latitude;
  readings[currentIndex].longitude = longitude;
  readings[currentIndex].timestamp = millis();
  currentIndex = (currentIndex + 1) % 10; // Keep the index within the range of 0-9
  if (readingCount < 10) {
    readingCount++;
  }
}

void sendHtmlResponse(WiFiClient &client, String message) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html; charset=UTF-8"); // Specify UTF-8 encoding
  client.println("Connection: close");
  client.println();

  IPAddress IP = WiFi.softAPIP();
  client.println("<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
  client.println("body { background-color: #303030; color: #ffffff; }</style>");
  client.println("<script>");
  client.println("function updateData() {");
  client.println("  fetch('/data').then(response => response.json()).then(data => {");
  client.println("    document.getElementById('moisture').innerText = data.current.moisture + ' %';");
  client.println("    document.getElementById('temperature').innerText = data.current.temperature + ' °C';");
  client.println("    document.getElementById('light').innerText = data.current.lightIntensity + ' %';");
  client.println("    document.getElementById('latitude').innerText = data.current.latitude;");
  client.println("    document.getElementById('longitude').innerText = data.current.longitude;");
  client.println("  });");
  client.println("}");
  client.println("setInterval(updateData, 2000)"); // Update every 2 seconds
  client.println("</script></head>");
  client.println("<body onload=\"updateData()\"><h1>ESP32 Agriculture Data</h1>");
  client.print("<p>AP IP Address: ");
  client.print(IP);
  client.println("</p>");
  client.println("<button onclick=\"location.href='/measure'\">Measure</button>");
  client.println("<button onclick=\"location.href='/table'\">Data</button>");
  if (message != "") {
    client.print("<p>");
    client.print(message);
    client.println("</p>");
  }
  client.print("<p>Moisture: <span id=\"moisture\">");
  client.print(realTimeMoisture);
  client.println("</span></p>");
  client.print("<p>Soil Temperature: <span id=\"temperature\">");
  client.print(realTimeTemperature);
  client.println("</span></p>");
  client.print("<p>Light Intensity: <span id=\"light\">");
  client.print(realTimeLightIntensity);
  client.println("</span></p>");
  client.print("<p>Latitude: <span id=\"latitude\">");
  client.print(realTimeLatitude);
  client.println("</span></p>");
  client.print("<p>Longitude: <span id=\"longitude\">");
  client.print(realTimeLongitude);
  client.println("</span></p>");
  client.println("</body></html>");
  client.println();
}

void sendDataJsonResponse(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type: application/json; charset=UTF-8");
  client.println("Connection: close");
  client.println();
  client.print("{");
  client.print("\"current\": {");
  client.print("\"temperature\": ");
  client.print(realTimeTemperature);
  client.print(", \"moisture\": ");
  client.print(realTimeMoisture);
  client.print(", \"lightIntensity\": ");
  client.print(realTimeLightIntensity);
  client.print(", \"latitude\": ");
  client.print(realTimeLatitude);
  client.print(", \"longitude\": ");
  client.print(realTimeLongitude);
  client.println("}");
  client.print("}");
}

void sendTableResponse(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html; charset=UTF-8");
  client.println("Connection: close");
  client.println();

  client.println("<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
  client.println("body { background-color: #303030; color: #ffffff; }");
  client.println("table { width: 100%; margin-top: 20px; border-collapse: collapse; }");
  client.println("table, th, td { border: 1px solid white; padding: 8px; text-align: center; }");
  client.println("</style></head>");
  client.println("<body><h1>ESP32 Agriculture Data</h1>");
  client.println("<button onclick=\"location.href='/'\">Back</button>");
  client.println("<table><tr><th>Timestamp</th><th>Moisture (%)</th><th>Temperature (°C)</th><th>Light Intensity (%)</th><th>Latitude</th><th>Longitude</th></tr>");
  
  for (int i = 0; i < readingCount; i++) {
    client.print("<tr><td>");
    client.print(readings[i].timestamp);
    client.print("</td><td>");
    client.print(readings[i].moisture);
    client.print("</td><td>");
    client.print(readings[i].temperature);
    client.print("</td><td>");
    client.print(readings[i].lightIntensity);
    client.print("</td><td>");
    client.print(readings[i].latitude);
    client.print("</td><td>");
    client.print(readings[i].longitude);
    client.println("</td></tr>");
  }
  
  client.println("</table></body></html>");
  client.println();
}

/*
#define LDR_PIN 35  // The ESP8266 pin connected to the light sensor

void setup() {
  Serial.begin(115200);
  pinMode(LDR_PIN, INPUT); 
}

void loop() {
  int sensorValue = analogRead(LDR_PIN); // read the value on analog pin
  int intensity = map(sensorValue, 39, 4095, 0, 100);  // Map to 0-100 scale
  Serial.print("Light Intensity: ");
  Serial.print(intensity);
  Serial.println("%");
  delay(2000);
}
*/
