#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>
#include <Adafruit_BMP085_U.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <RemoteDebug.h>
#include <WiFiUdp.h>


#define ONE_WIRE_BUS 2  // DS18B20 on arduino pin2 corresponds to D4 on physical board
#define sda D2
#define scl D1

const char* SSID = "SFR_B0C8"; 
const char* PWD = "evosethertuo4uzeillo";

String server = "http://192.168.1.26";

Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature temperatureSensors(&oneWire);

RemoteDebug Debug;

void displaySensorDetails(void)
{
  sensor_t sensor;
  tsl.getSensor(&sensor);

  println("------------------------------------");
  print  ("Sensor:       "); println(sensor.name);
  print  ("Driver Ver:   "); println(sensor.version);
  print  ("Unique ID:    "); println(sensor.sensor_id);
  print  ("Max Value:    "); print(sensor.max_value); println(" lux");
  print  ("Min Value:    "); print(sensor.min_value); println(" lux");
  print  ("Resolution:   "); print(sensor.resolution); println(" lux");
  println("------------------------------------");
  println("");
  
  sensor_t sensorBmp085;
  bmp.getSensor(&sensorBmp085);

  println("------------------------------------");
  print  ("Sensor:       "); println(sensorBmp085.name);
  print  ("Driver Ver:   "); println(sensorBmp085.version);
  print  ("Unique ID:    "); println(sensorBmp085.sensor_id);
  print  ("Max Value:    "); print(sensorBmp085.max_value); println(" hPa");
  print  ("Min Value:    "); print(sensorBmp085.min_value); println(" hPa");
  print  ("Resolution:   "); print(sensorBmp085.resolution); println(" hPa");  
  println("------------------------------------");
  println("");
  
  delay(500);
}

void configureTsl2561Sensor(void)
{
  tsl.enableAutoRange(true);            /* Auto-gain ... switches automatically between 1x and 16x */
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);      /* fast but low resolution */

  println("------------------------------------");
  print  ("Gain:         "); println("Auto");
  print  ("Timing:       "); println("13 ms");
  println("------------------------------------");
}

void connectWifi()
{
  print("Connecting to " + *SSID);
  WiFi.begin(SSID, PWD);
  println("MAC ADDRESS | " + WiFi.macAddress());

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    print(".");
  }
  
  println("Connected"); 
}

void print(float message){
  Debug.print(String(message));
  Serial.print(String(message)); 
}

void print(String message){
  Debug.print(message);
  Serial.print(message); 
}

void println(float message){
  Debug.println(String(message));
  Serial.println(String(message)); 
}

void println(String message){
  Debug.println(message);
  Serial.println(message); 
}

void setup(void)
{
  Serial.begin(115200);
  connectWifi();
  
  Debug.begin("Sensor Temperature/Lux/Pressure");  
  ArduinoOTA.setHostname("Sensor Temperature/Lux/Pressure");
  ArduinoOTA.begin();
  
  Wire.begin(sda, scl);
  temperatureSensors.begin();

  if (!tsl.begin())
  {
    /* There was a problem detecting the TSL2561 ... check your connections */
    print("Ooops, no TSL2561 detected ... Check your wiring or I2C ADDR!");
    while (1);
  }
  
  if(!bmp.begin())
  {
    /* There was a problem detecting the BMP085 ... check your connections */
    print("Ooops, no BMP085 detected ... Check your wiring or I2C ADDR!");
    while(1);
  }
  
  displaySensorDetails();
  configureTsl2561Sensor();

  println("Setup ok");
}

void sendData(String serie, String sensorName, float sensorValue)
{
  HTTPClient http;
  http.begin(server + ":8086/write?db=personalHome");

  String postData = String(serie) + ",name=" + String(sensorName) + " value=" + String(sensorValue);
  int httpCode = http.POST(postData);
  println("HTTP Code | " + String(httpCode) + "| Payload | " + postData);
  http.end();
}

void loop(void)
{
  temperatureSensors.requestTemperatures();
  float temperature = temperatureSensors.getTempCByIndex(0);
  if  (temperature < 80 && temperature > -30)
  {
    sendData("temperature", "home", temperature);
  }

  sensors_event_t eventLuminosity;
  tsl.getEvent(&eventLuminosity);
  if (eventLuminosity.light)
  {
    sendData("luminosity", "home", eventLuminosity.light);
  }
  else
  {
    println("Sensor overload");
  }

  sensors_event_t event;
  bmp.getEvent(&event);
 
  if (event.pressure)
  {
    float seaLevelPressure = SENSORS_PRESSURE_SEALEVELHPA;
    float temperature;
    bmp.getTemperature(&temperature);
    sendData("bmp085 temperature", "home", temperature);
    sendData("pressure", "home", event.pressure);
    sendData("altitude", "home", bmp.pressureToAltitude(seaLevelPressure,temperature, event.pressure));
  }
  else
  {
    println("Sensor error");
  }

  ArduinoOTA.handle(); 
  delay(15000);
}
