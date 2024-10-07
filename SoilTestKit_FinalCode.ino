#include "Sensor.h"

void setup() {
  Serial.begin(115200);
  setupSoilSensor();
}


void loop() {
  MeasurementData data;
  measure(&data);
  // Print the data to the Serial monitor
  Serial.print("Temperature: ");
  Serial.print(data.temperature);
  Serial.print(" °C, Moisture: ");
  Serial.print(data.moisture);
  Serial.print(" %, Conductivity: ");
  Serial.print(data.conductivity);
  Serial.print(" uS/cm, pH: ");
  Serial.print(data.pH);
  Serial.print(", N: ");
  Serial.print(data.N);
  Serial.print(" mg/kg, P: ");
  Serial.print(data.P);
  Serial.print(" mg/kg, K: ");
  Serial.print(data.K);
  Serial.print(" mg/kg, Light Intensity: ");
  Serial.print(data.light_intensity);
  Serial.println(" %");
  
  delay(5000); // Wait for 5 seconds before the next measurement
}
