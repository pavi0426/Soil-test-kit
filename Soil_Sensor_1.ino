#include<HardwareSerial.h>

#define modbus 2;
#define TX_2 17;
#define RX_2 16;


const byte temp[] = {0x01,0x03, 0x00, 0x13, 0x00, 0x01, 0x75, 0xcf};
const byte mois[]  = {0x01,0x03,0x00,0x12,0x00,0x01,0x24,0x0F};
const byte econ[] = {0x01,0x03, 0x00, 0x15, 0x00, 0x01, 0x95, 0xce};
const byte ph[] = {0x01,0x03, 0x00, 0x06, 0x00, 0x01, 0x64, 0x0b};
const byte nitro[] = { 0x01, 0x03, 0x00, 0x1E, 0x00, 0x01, 0xE4, 0x0C };
const byte phos[] = { 0x01, 0x03, 0x00, 0x1f, 0x00, 0x01, 0xb5, 0xcc };
const byte pota[] = { 0x01, 0x03, 0x00, 0x20, 0x00, 0x01, 0x85, 0xc0 };

byte Sensor_Values[11];

float temp = 0, moisture = 0, conductivity = 0, pH = 0, N = 0, P = 0, K = 0;

void setup()
{
  Serial.begin(115200);
  Serial_Soil_Sensor.begin(9600, SERIAL_8N1, RX_2, TX_2);
  pinMode(modbus, OUTPUT);
}

//function to return temperature value
float measure_temp(void)
{
  digitalWrite(modbus, HIGH);
  if(Serial_Soil_Sensor.available())
  {
    for(uint8_t i = 0; i < sizeof(temp); i++)
    {
      Serial_Soil_Sensor.write(temp[i]);
    }
  }
  
  digitalWrite(modbus, LOW);
  if(Serial_Soil_Sensor.available())
  {
    for(uint8_t i = 0; i < sizeof(temp); i++)
    {
      Serial_Soil_Sensor.read();
    }
  }
}

void loop()
{

}