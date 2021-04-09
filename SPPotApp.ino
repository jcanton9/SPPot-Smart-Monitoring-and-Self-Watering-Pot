/*
 SPPotApp

 This code will create a web server that will print out sensor data coming from the SPPot. 
 It also sets the pot to check data within the Serial monitor as well as making the pot functional.
 The arduino secrets header file gets the wifi name and password of a user that they input between
 the quotation marks. They can then successfully connect their pot to the Wi-Fi and have the IP
 of the pot printed out through the Serial monitor. First-time users should always run this
 code in order to setup their Wi-Fi network as well as knowing the IP of the pot. This code
 also has specific client requests that are used to get the soil type selected by the user. 
 
 Created by Team 34 Senior Design FIU
 */
#include <SPI.h>
#include <WiFiNINA.h>
#include <DHT.h>
#include "arduino_secrets.h"
const int ledPin = 2;                                             // Digital output pin that the LED is attached to
const int pumpPin = 12;                                           // Digital output pin that the water pump is attached to
const int waterLevelPin = A3;                                      // Analoge pin water level sensor is connected to
const int moistureSensorPin = A4;                                  // Digital input pin used to check the moisture level of the soil
#define DHTPIN 5     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor for normal 16mhz Arduino

// These are the values to edit - see the instructional video to find out what needs adjusting and why:

double checkInterval = 18000;                                      //time to wait before checking the soil moisture level - default it to an hour = 1800000
int waterLevelThreshold = 0;                                    // threshold at which we flash the LED to warn you of a low water level in the pump tank - set this as per the video explains
int emptyReservoirTimer = 90;                                     // how long the LED will flash to tell us the water tank needs topping up - default it to 900 = 30mins
int amountToPump = 50;                                           // how long the pump should pump water for when the plant needs it
int chk;
float hum;  //Stores humidity value
float temp; //Stores temperature value
String currentLine = "";

// Global temp values

int sensorWaterLevelValue = 0;                                      // somewhere to store the value read from the waterlevel sensor
int moistureSensorValue = 0; 
int soilDry = 0;
int soilWet = 0;   
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                 // your network key index number (needed only for WEP)

int status = WL_IDLE_STATUS;
WiFiServer server(8088);

void setup() {
  Serial.begin(9600);      // initialize serial communication
  pinMode(9, OUTPUT);      // set the LED pin mode

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);                   // print the network name (SSID);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);
  }
  server.begin();                           // start the web server on port 80
  printWifiStatus();                        // you're connected now, so print out the status

      pinMode(ledPin, OUTPUT);
  pinMode(pumpPin, OUTPUT);
  pinMode(moistureSensorPin, INPUT);
  dht.begin();

                                                                //flash the LED five times to confirm power on and operation of code:
   for (int i=0; i <= 4; i++){
      digitalWrite(ledPin, HIGH);
      delay(300);
      digitalWrite(ledPin, LOW);
      delay(300);
    }
    delay(2000);

  digitalWrite(ledPin, LOW);                                   // turn the LED on 
}


void loop() {

  
  WiFiClient client = server.available();   // listen for incoming clients
  SPPotProgram();
  if (client) {                             // if you get a client,
    Serial.println("new client");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {

            // The HTTP response ends with another blank line:
            
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Refresh: 20");
            client.println();
            client.println("<!DOCTYPE HTML>");
            client.println("<html>");
            client.print(dht.readTemperature());//prints the temperature
            client.println("<br />");//prints new line
            client.print(dht.readHumidity());//prints the humidity
            client.println("<br />");
            client.print(analogRead(waterLevelPin));//prints the water level
            client.println("<br />");
            client.print(analogRead(moistureSensorPin));//prints the soil moisture
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /Clay" which will then change the soilDry and SoilWet values
          if (currentLine.endsWith("GET /Clay")) 
          {
            soilDry = 650;
            soilWet = 400;

          }
          else if (currentLine.endsWith("GET /Silt")) 
          {
            soilDry = 700;
            soilWet = 450;            
          }
          else if (currentLine.endsWith("GET /Peaty")) 
          {
            soilDry = 800;
            soilWet = 550;     
          }
          else if (currentLine.endsWith("GET /Loamy")) 
          {
            soilDry = 750;
            soilWet = 500;            
          }
          else if (currentLine.endsWith("GET /Sandy")) 
          {
            soilDry = 850;
            soilWet = 600;            
          }
        }   
    }
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}

//This part of the code will run the normal operation of the SPPot
void SPPotProgram() {
    hum = dht.readHumidity();
    temp= dht.readTemperature();
    //Print temp and humidity values to serial monitor
    Serial.print("Humidity: ");
    Serial.print(hum);
    Serial.print(" %, Temp: ");
    Serial.print(temp);
    Serial.println(" Celsius");
    sensorWaterLevelValue = analogRead(waterLevelPin);              //read the value of the water level sensor
    Serial.print("Water level sensor value: ");                     //print it to the serial monitor
    Serial.println(sensorWaterLevelValue);
    if (sensorWaterLevelValue < waterLevelThreshold){               //check if we need to alert you to
    Serial.println(sensorWaterLevelValue); // low water level in the tank
        for (int i=0; i <= emptyReservoirTimer; i++){  
          digitalWrite(ledPin, LOW);
          delay(1000);
          digitalWrite(ledPin, HIGH);
          delay(1000);
        }
    }
    else {
      digitalWrite(ledPin, HIGH);
      delay(checkInterval);                                         //wait before checking the soil moisture level
    }


// check soil moisture level

    moistureSensorValue = analogRead(moistureSensorPin);       //read the moisture sensor and save the value
    Serial.print("Soil moisture sensor is currently: ");
    Serial.print(moistureSensorValue);
    Serial.print(" ('");
    Serial.print(soilDry);
    Serial.print(" ' means soil is too dry and '");
    Serial.print(soilWet);
    Serial.println("' means the soil is moist enough.)");
    if(soilDry != 0 && soilWet != 0){
      if (moistureSensorValue > soilDry){
                                                                 //pulse the pump 
        digitalWrite(pumpPin, HIGH);
          Serial.println("pump on");
        delay(amountToPump);                                      //keep pumping water
        digitalWrite(pumpPin, LOW);
         Serial.println("pump off");
        delay(800);                                              //delay to allow the moisture in the soil to spread through to the sensor
      }
      else if(moistureSensorValue < soilWet) {
        digitalWrite(pumpPin, LOW);
        Serial.println("The soil is too wet");
      }
    }     
}
void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.print(ip);
  Serial.println(":8088");
}
