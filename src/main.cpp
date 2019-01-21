#include <Arduino.h>
#include <PubSubClient.h>
#include "Gsender.h"
#include "ESPBASE.h"
#include "OneWire.h"
#include "DallasTemperature.h"

OneWire oneWire(4);
DallasTemperature DS18B20(&oneWire);
long lastReconnectAttempt = 0;
String StatusTopic;
String sChipID;
long lastTemp = 0; //The last measurement
DeviceAddress devAddr[15];  //An array device temperature sensors
const int durationTemp = 5000; //The frequency of temperature measurement
int numberOfDevices; //Number of temperature devices found
float tempDev[15]; //Saving the last measurement of temperature
float tempDevLast[15]; //Previous temperature measurement
Gsender *gsender;
ESPBASE Esp;
bool mailSent = false;

String GetAddressToString(DeviceAddress deviceAddress)
{
  String str = "";
  for (uint8_t i = 0; i < 8; i++)
  {
    if( deviceAddress[i] < 16 ) str += String(0, HEX);
    str += String(deviceAddress[i], HEX);
  }
  return str;
}

//Setting the temperature sensor
void SetupDS18B20(){
//  Serial.begin(115200);
  DS18B20.begin();

  Serial.print("Parasite power is: "); 
  if( DS18B20.isParasitePowerMode() ){ 
    Serial.println("ON");
  }else{
    Serial.println("OFF");
  }
  
  numberOfDevices = DS18B20.getDeviceCount();
  Serial.print( "Device count: " );
  Serial.println( numberOfDevices );

  lastTemp = millis();
  DS18B20.requestTemperatures();

  // Loop through each device, print out address
  for(int i=0;i<numberOfDevices; i++){
    // Search the wire for address
    if( DS18B20.getAddress(devAddr[i], i) ){
      //devAddr[i] = tempDeviceAddress;
      Serial.print("Found device ");
      Serial.print(i, DEC);
      Serial.print(" with address: " + GetAddressToString(devAddr[i]));
      Serial.println();
    }else{
      Serial.print("Found ghost device at ");
      Serial.print(i, DEC);
      Serial.print(" but could not detect address. Check power and cabling");
    }

    //Get resolution of DS18b20
    Serial.print("Resolution: ");
    Serial.print(DS18B20.getResolution( devAddr[i] ));
    Serial.println();

    //Read temperature from DS18b20
    float tempC = DS18B20.getTempC( devAddr[i] );
    Serial.print("Temp C: ");
    Serial.println(tempC);
  }
}

void setup() {
  Serial.begin(115200);
  char cChipID[10];
  sprintf(cChipID,"%08X",ESP.getChipId());
  sChipID = String(cChipID);

  Esp.initialize();
  StatusTopic = String(DEVICE_TYPE) + "/" + config.DeviceName + "/status";
  customWatchdog = millis();
  SetupDS18B20();

  Serial.println("Done with setup");
  gsender = Gsender::Instance();    // Getting pointer to class instance
/*  test mail
  Gsender *gsender = Gsender::Instance();    // Getting pointer to class instance
  String subject = "Subject is optional!";
  if(gsender->Subject(subject)->Send("5303133307@tmomail.net", "Setup test")) {
      Serial.println("Message send.");
  } else {
      Serial.print("Error sending message: ");
      Serial.println(gsender->getError());
  } */
}

//Loop measuring the temperature
void TempLoop(long now)
{
  String msgline;
  if( now - lastTemp > durationTemp )
  { //Take a measurement at a fixed time (durationTemp = 5000ms, 5s)
//    Serial.println(numberOfDevices);
    if(numberOfDevices == 0)
    {
      Serial.print("reset = ");Serial.println(oneWire.reset());
      DeviceAddress deviceAddress;

      oneWire.reset_search();
      while (oneWire.search(deviceAddress)) 
      {
        Serial.println("got address");
      }
      DS18B20.begin();
      numberOfDevices = DS18B20.getDeviceCount();
      
      Serial.print( "Device count: " );
      Serial.println( numberOfDevices );
    }
    msgline = "";
    for(int i=0; i<numberOfDevices; i++)
    {
      float tempF = DS18B20.getTempF( devAddr[i] ); //Measuring temperature in Celsius
      tempDev[i] = tempF; //Save the measured value to the array
      if(tempF > 32.0 && !mailSent)
      {
        mailSent = true;
        String subject = "Freezer temperature warning";
        if(gsender->Subject(subject)->Send("5303133307@tmomail.net", "Freezer Temp is " + String(tempF))) 
        {
          Serial.println("Message send.");
        } 
        else 
        {
          Serial.print("Error sending message: ");
          Serial.println(gsender->getError());
        } 
      }
      if(tempF < 32.0 && mailSent)
      {
        mailSent = false;
        String subject = "Freezer temperature ok";
        if(gsender->Subject(subject)->Send("5303133307@tmomail.net", "Freezer Temp is " + String(tempF))) 
        {
          Serial.println("Message send.");
        } 
        else 
        {
          Serial.print("Error sending message: ");
          Serial.println(gsender->getError());
        } 
      }
//      msgline = msgline + String(i) + "| Temp F: " + String(tempF) + " ";
//      handleTemp(i,tempF);
      Esp.mqttSend(String(DEVICE_TYPE) + "/" + config.DeviceName + "/value","",String(i) + ":" + String(tempF)); 
    }
//    msgline = msgline + "| relay 1 = " + String(relay1state) + " | relay 2 = " + String(relay2state);
    Serial.println(msgline);
    DS18B20.setWaitForConversion(false); //No waiting for measurement
    DS18B20.requestTemperatures(); //Initiate the temperature measurement
    lastTemp = millis();  //Remember the last time measurement
  }
}

void loop() {
  Esp.loop();
  long t=millis();
  TempLoop(t);
}

String getSignalString()
{
  String signalstring = "";
  byte available_networks = WiFi.scanNetworks();
  signalstring = signalstring + sChipID + ":";
 
  for (int network = 0; network < available_networks; network++) {
    String sSSID = WiFi.SSID(network);
    if(network > 0)
      signalstring = signalstring + ",";
    signalstring = signalstring + WiFi.SSID(network) + "=" + String(WiFi.RSSI(network));
  }
  return signalstring;    
}

void sendStatus()
{
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char c_payload[length];
  memcpy(c_payload, payload, length);
  c_payload[length] = '\0';
  
  String s_topic = String(topic);
  String s_payload = String(c_payload);
  Serial.print(s_topic + ":" + s_payload);
}

void mqttSubscribe()
{
    if (Esp.mqttClient->connected()) 
    {
        //subscribe to topics here
    }
}


