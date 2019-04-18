 

#include <ESP8266WiFi.h>
#include "HTTPSRedirect.h"

const char* ssid = "SSID";
const char* password = "PASSWORD";

// The ID below comes from Google Sheets.
// Towards the bottom of this page, it will explain how this can be obtained
const char *GScriptId = "AKfycbwgTt0-fTb1hjPhx0KhLnIvOKAWkfpc_lSIo3T6n8-jc7XeZiTy";

// Push data on this interval
const int dataPostDelay = 2000;  // 1 minutes = 1 * 60 * 1000;

const char* host = "script.google.com";
const char* googleRedirHost = "script.googleusercontent.com";

const int httpsPort =  443;
HTTPSRedirect client(httpsPort);

// Prepare the url (without the varying data)
String url = String("/macros/s/") + GScriptId + "/exec?";

const char* fingerprint = "97 E1 03 DA DC 42 20 55 BE 8B DA F2 D3 B6 52 CE 4A D1 B3 B8";
//Fingerprint=97:E1:03:DA:DC:42:20:55:BE:8B:DA:F2:D3:B6:52:CE:4A:D1:B3:B8

//SENSORS
#include <TinyGPS++.h>
unsigned long start;
int dateYear, dateMonth, dateDay;
uint8_t timeHour, timeMinute, timeSecond;

double latValue, lngValue;
static const int RX=12, TX=13; //RX=D6, TX=D7
TinyGPSPlus gps;//TinyGPS++ object
static const uint32_t GPSBaud = 9600;
#include <SoftwareSerial.h>
SoftwareSerial GPSss(RX, TX);

#include "DHT.h"
#define DHTPin 2 //D4
#define DHTTYPE DHT22
float tempValue, humidValue, heatIndex, fTemp, fIndex;
DHT dht(DHTPin, DHTTYPE);//initialize DHT sensor

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x3F,16,2);//SDA to D2 & SCL to D1
#include <Wire.h>

//const int ppm_sensor = A0;
//int ppmValue;
#include "MQ135.h"
#define ANALOGPIN A0    //  Define Analog PIN on Arduino Board
#define RZERO 1929.734   //  Define RZERO Calibration Value
MQ135 gasSensor = MQ135(ANALOGPIN);
float ppm;

 

void setup() {
  Serial.println("AIR QUALITY MONITOR");
  Serial.begin(115200);
  GPSss.begin(GPSBaud);//initialize SoftwareSerial at defined GPSBaud
  dht.begin();//initialize DHT sensor
  float rzero = gasSensor.getRZero();//Calculating Rs for finding actual ppm
  Serial.print("MQ135 RZERO Calibration Value : ");
  Serial.println(rzero);
  //pinMode(ppm_sensor,INPUT);
  
  connectToWifi();
}

void connectToWifi(){
  Serial.println("Connecting to wifi: ");
  Serial.println(ssid);
  Serial.flush();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" IP address: ");
  Serial.println(WiFi.localIP());

  
  Serial.print(String("Connecting to "));
  Serial.println(host);

  bool flag = false;
  for (int i=0; i<5; i++){
    int retval = client.connect(host, httpsPort);
    if (retval == 1) {
       flag = true;
       break;
    }
    else
      Serial.println("Connection failed. Retrying...");
  }

  // Connection Status, 1 = Connected, 0 is not.
  Serial.println("Connection Status: " + String(client.connected()));
  Serial.flush();
  
  if (!flag){
    Serial.print("Could not connect to server: ");
    Serial.println(host);
    Serial.println("Exiting...");
    Serial.flush();
    return;
  }

  // Data will still be pushed even certification don't match.
  if (client.verify(fingerprint, host)) {
    Serial.println("Certificate match.");
  } else {
    Serial.println("Certificate mis-match");
  }
}


// This is the main method where data gets pushed to the Google sheet
void postData(float Latitude, float Longitude, float PPM, float Temperature, float Humidity)
//String tag, float value
{
  //float Latitude, float Longitude, float PPM, float Temperature, float Humidity
  if (!client.connected()){
    Serial.println("Connecting to client again..."); 
    client.connect(host, httpsPort);
  }
  String urlFinal = url + "Latitude=" + String(Latitude) 
                        + "&Longitude=" + String(Longitude) 
                        + "&PPM=" + String(PPM) 
                        + "&Temperature=" + String(Temperature) 
                        + "&Humidity=" + String(Humidity);
  client.printRedir(urlFinal, host, googleRedirHost);
 Serial.print("URL generated:-");
  Serial.println(urlFinal);
}

// Continue pushing data at a given interval
void loop() {
  readData();
  
  // Read analog value, in this case a soil moisture
  //int data = 1023 - analogRead(AnalogIn);

  // Post these information
  postData(latValue, lngValue, ppm, heatIndex, humidValue);
  delay (dataPostDelay);
}


void readData(){
  tempValue= dht.readTemperature();//read temperature in Celcius
  //fTemp= dht.readTemperature(true);//read temperature in Fahrenheit
  humidValue=dht.readHumidity();//read humidity
  heatIndex= dht.computeHeatIndex(tempValue, humidValue, false);//apparent heat
  //fIndex= dht.computeHeatIndex(fTemp, humidValue);
  //ppmValue= analogRead(ppm_sensor);
  ppm = gasSensor.getPPM();//finding ppm using Rs and R0 in the formula:-ppm = a*(Rs/Ro)^b
  gpsRead();
  
}

void gpsRead(){
  delay(1000);
 // This sketch displays information every time a new sentence is correctly encoded.
  while (GPSss.available() > 0)
    if (gps.encode(GPSss.read()))
      displayInfo();

  if (millis() > 5000 && gps.charsProcessed() < 10)
  { 
    Serial.println(F("No GPS detected: check wiring."));
    while(true);
  }
}

void displayInfo()
{
  Serial.print(F("Location: ")); 
  if (gps.location.isValid())
  {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
  }
  else
  {
    Serial.print(F("INVALID"));
  }
   Serial.println();
}

