//DHT22 Sensor
#include "DHT.h"
#define DHTPin D4 //GPIO2
#define DHTTYPE DHT22
float tempValue, humidValue, heatIndex, fTemp, fIndex;
DHT dht(DHTPin, DHTTYPE);//initialize DHT sensor

//////////////////////////////////////////////////
//LCD Display

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x3F,16,2);//SDA to D2 & SCL to D1
#include <Wire.h>

#include <ESP8266WiFi.h>
#include "HTTPSRedirect.h"

const char* ssid = "SSID";
const char* password = "PASSWORD";

// The ID below comes from Google Sheets.
// Towards the bottom of this page, it will explain how this can be obtained
const char *GScriptId = "AKfycbwDJ-3vbE5qBr35qZZZ3tD0GMHjd_g734s5jWiVApzZGr1RFlo";
//"AKfycbwgTt0-fTb1hjPhx0KhLnIvOKAWkfpc_lSIo3T6n8-jc7XeZiTy";

// Push data on this interval
const int dataPostDelay = 2000;  // 1 minutes = 1 * 60 * 1000;
const int sensor_relax_time = 10000;//MQ135 sensor OFF time, since it is used for calculation of two gases hence in between it needs to relax

const char* host = "script.google.com";
const char* googleRedirHost = "script.googleusercontent.com";

const int httpsPort =  443;
HTTPSRedirect client(httpsPort);

// Prepare the url (without the varying data)
String url = String("/macros/s/") + GScriptId + "/exec?";

const char* fingerprint = "0C DA AB FB 6D 13 5E 3E 82 C7 4B 54 0B 04 F0 7D BF FD 42 FB";//home
//Fingerprint= 29 40 55 83 01 36 FA 5D BD 5E 6B 91 58 29 EA 8F 26 7E 22 0B;//dic


///////////////////////////////////////////
//PPD42NS PM particle counter
int pin25 = D7;
unsigned long duration25;
unsigned long starttime25;
unsigned long sampletime_ms = 30000;//30 seconds
unsigned long lowpulseoccupancy25 = 0;
float ratio25 = 0;
float concentration25 = 0;

int pin1 = D6;
unsigned long duration1;
unsigned long starttime1;
unsigned long lowpulseoccupancy1 = 0;
float ratio1 = 0;
float concentration1 = 0;

float particle_weight = 12 * pow(10,-6);  //Weight of a single piece of dust in micrograms
float k = 3531.5; //conversion constant to convert data from 0.01cf(cubic feet) to m3(cubic metre)
float PM25, PM1;

///////////////////////
//unsigned long calculation_delay = 0;

///////////////////////////////////////////////

//MQ135 sensor
#define MQ135_sensor_pin A0
#define CO2_PARA 112.91
#define CO2_PARB -2.90
#define CO_PARA 568.10
#define CO_PARB -3.93
//#define NH4_PARA 101.81
//#define NH4_PARB -2.5

//#define defined_CO2_PPM 400
//#define defined_CO_PPB 238

#define RL 33;//load resistance in kilo-ohms
#define Ro_CO2 1257.52
#define Ro_CO 647.8
float CO2_PPM_avg, CO_PPM_avg;


//////////////////////////////////////////////////////
void setup() {
  Serial.begin(115200);
  pinMode(pin25,INPUT);
  pinMode(pin1,INPUT);
  

  dht.begin();
  
  lcd.begin();
  lcd.setCursor(3,0);
  lcd.print("AIR QUALITY");
  lcd.setCursor(5,1);
  lcd.print("MONITOR");
  delay(5000);
  connectToWifi();
  
  //starttime25 = millis();//get the current time;
  //starttime1 = millis();//get the current time
}

//////////////////////////////////////////////////////
void loop(){
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Calculating...");
  calculate_data();
  
  //calculation_delay= millis();
  
  lcd_display_data();
  Serial.println("#########################################################################");
  
}

//////////////////////////////////////////////////////
void connectToWifi(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Connecting...");
  delay(1000);
  Serial.println("Connecting to wifi: ");
  Serial.println(ssid);
  Serial.flush();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("IP address: ");
  lcd.setCursor(0,1);
  delay(2000);
  lcd.print(WiFi.localIP());
  Serial.println(" IP address: ");
  Serial.println(WiFi.localIP());

  //unsigned long connecting_time = millis();
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
/*
  sampletime_ms += connecting_time;
  Serial.print("Sampletime for particle sensor= ");
  Serial.println(sampletime_ms);
*/
  
  // Data will still be pushed even certification don't match.
  if (client.verify(fingerprint, host)) {
    Serial.println("Certificate match.");
  } else {
    Serial.println("Certificate mis-match");
  }
}

/////////////////////////////////////////////////////////////
void postData(float Temperature, float Humidity, float PMcount1, float PMcount25, float PM25, float CO2_conc, float CO_conc)
//String tag, float value
{
  if (!client.connected()){
    Serial.println("Connecting to client again..."); 
    client.connect(host, httpsPort);
  }
  String urlFinal = url + "&Temperature=" + String(Temperature) + "&Humidity=" + String(Humidity) + "&PMcount1=" + String(PMcount1) + "&PMcount25=" + String(PMcount25) + "&PM25=" + String(PM25) + "&CO2_conc=" + String(CO2_conc) + "&CO_conc=" + String(CO_conc); 

  client.printRedir(urlFinal, host, googleRedirHost);
  Serial.print("URL generated:-");
  Serial.println(urlFinal);
}

/////////////////////////////////////////////////////////////
void calculate_data(){
  DHT22_sensor_data();
  MQ135_sensor_data();
  PPD42NS_sensor_data(); 
  //lcd_display_data();
}

/////////////////////////////////////////////////////////////
void DHT22_sensor_data(){
  tempValue= dht.readTemperature();//read temperature in Celcius
  //fTemp= dht.readTemperature(true);//read temperature in Fahrenheit
  humidValue= dht.readHumidity();//read humidity
  heatIndex= dht.computeHeatIndex(tempValue, humidValue, false);//apparent heat
  //fIndex= dht.computeHeatIndex(fTemp, humidValue);
  Serial.print("Temp.= ");
  Serial.print(heatIndex);
  Serial.print("  ,  ");
  Serial.print("Humidity= ");
  Serial.println(humidValue);
}

/////////////////////////////////////////////////////////////
void MQ135_sensor_data(){
  float Rs_CO2=0, RsAverage_CO2=0, CO2_PPM[100], x=0, y=0;
  float analogValue_CO2=0, RsValue_CO2=0, sensor_resistance_CO2[100];

for(int k=0; k<100; k++){
  for(int i=0;i<100;i++){
    analogValue_CO2 = analogRead(MQ135_sensor_pin);
    Rs_CO2 = ((1024/analogValue_CO2)-1)*RL;
    sensor_resistance_CO2[i]=Rs_CO2;
    //delay(1000);
  }
  for(int j=0; j<100; j++){
    RsValue_CO2 = RsValue_CO2 + sensor_resistance_CO2[j];
  }
  
  RsAverage_CO2 = RsValue_CO2/100;//Averaged sensor resistance 
  CO2_PPM[k] = CO2_PARA * pow((RsAverage_CO2/Ro_CO2),CO2_PARB);
}
for(int i=0;i<100;i++){
   x = x + CO2_PPM[i];
}
CO2_PPM_avg = x;
Serial.print("CO2 ppm= ");
Serial.print(CO2_PPM_avg);
delay(sensor_relax_time);

/*#################################################################*/

  float Rs_CO=0, RsAverage_CO=0, CO_PPM[100];;
  float analogValue_CO=0, RsValue_CO=0, sensor_resistance_CO[100];
  for(int z=0; z<100; z++){
  for(int i=0;i<100;i++){
    analogValue_CO = analogRead(MQ135_sensor_pin);
    Rs_CO = ((1024/analogValue_CO)-1)*RL;
    sensor_resistance_CO[i]=Rs_CO;
    //delay(1000);
  }
  for(int j=0; j<100; j++){
    RsValue_CO = RsValue_CO + sensor_resistance_CO[j];
  }
  
  RsAverage_CO = RsValue_CO/100;//Averaged sensor resistance
  CO_PPM[z] = CO_PARA * pow((RsAverage_CO/Ro_CO),CO_PARB);
  }
  for(int i=0;i<100;i++){
  y = y + CO_PPM[i];
}
CO_PPM_avg = y;
Serial.print("  ,  ");
Serial.print("CO ppb= ");
Serial.println(CO_PPM_avg);
}

/////////////////////////////////////////////////////////////

void PPD42NS_sensor_data(){
 starttime25 = millis();//get the current time
 starttime1 = millis();//get the current time 
 
 while((millis()-starttime25) < (sampletime_ms + 5000) && (millis()-starttime1) < (sampletime_ms + 5000))//if the sample time == 30s
 {
 duration25 = pulseIn(pin25, LOW);
 lowpulseoccupancy25 = lowpulseoccupancy25 + duration25;
 duration1 = pulseIn(pin1, LOW);
 lowpulseoccupancy1 = lowpulseoccupancy1 + duration1;
 //sampletime_ms += calculation_delay;
 //calculation_delay = 0; 
 
 if ((millis()-starttime25) > sampletime_ms && (millis()-starttime1) > sampletime_ms)//if the sample time == 30s
 {
 ratio25 = lowpulseoccupancy25/(sampletime_ms*10.0); // Integer percentage 0=>100
 concentration25 = 1.1*pow(ratio25,3)-3.8*pow(ratio25,2)+520*ratio25+0.62; // using spec sheet curve
 ratio1 = lowpulseoccupancy1/(sampletime_ms*10.0); // Integer percentage 0=>100
 concentration1 = 1.1*pow(ratio1,3)-3.8*pow(ratio1,2)+520*ratio1+0.62; // using spec sheet curve

 PM25 = concentration25 * 3531.5 * particle_weight;//Convert particle conc.(pcs/0.01cu feet) to particle concentration(mg/m3)

 //********************************************************************************
 Serial.println("Posting data to sheet....");
 postData(heatIndex, humidValue, concentration1, concentration25, PM25, CO2_PPM_avg, CO_PPM_avg);//post complete 
 delay(dataPostDelay);
 //********************************************************************************
  
 lowpulseoccupancy25 = 0;
 //starttime25 = millis();
 starttime25 = 0;
 lowpulseoccupancy1 = 0;
 starttime1 = 0;
 //starttime1 = millis();
 //sampletime_ms=30000;
}
 }
  Serial.print("PM1 concentration = ");
  Serial.print(concentration1);
  Serial.print(" pcs/0.01cf , ");
  Serial.print("PM2.5 concentration = ");
  Serial.print(concentration25);
  Serial.print(" pcs/0.01cf , ");
  Serial.print(PM25);
  Serial.println(" ug/m3");
  Serial.println();
}

/////////////////////////////////////////////////////////////
void lcd_display_data(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Printing Data...");
  delay(2000);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp.=");
  lcd.setCursor(7, 0);
  lcd.print(heatIndex);
  lcd.setCursor(11,0);
  lcd.print("*C");
  lcd.setCursor(0, 1);
  lcd.print("Humid.=");
  lcd.setCursor(8,1);
  lcd.print(humidValue);
  lcd.setCursor(12,1);
  lcd.print("%");
  delay(5000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CO2=");
  lcd.setCursor(5, 0);
  lcd.print(CO2_PPM_avg);
  lcd.setCursor(0,1);
  lcd.print("CO=");
  lcd.setCursor(4,1);
  lcd.print(CO_PPM_avg);
  delay(5000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("PM2.5=");
  lcd.setCursor(7,0);
  lcd.print(PM25);
  lcd.setCursor(11,0);
  lcd.print("ug/m3");
  delay(5000);
  lcd.clear();

}
