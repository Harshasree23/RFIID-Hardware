 /*
  # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #                                                               #
  #                 Installation :                                      #
  # NodeMCU ESP8266/ESP12E    RFID MFRC522 / RC522                      #
  #         D4      <---------->   SDA/SS                               #
  #         D5       <---------->   SCK                                 #
  #         D7       <---------->   MOSI                                #
  #         D6       <---------->   MISO                                #
  #         GND      <---------->   GND                                 #
  #         D3       <---------->   RST                                 #
  #         3V/3V3   <---------->   3.3V                                #
  #         D0       <---------->   LED Yellow                          #
  #         D1       <---------->   LED orange                          #
  #         D2       <---------->   LED green                           #
  # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
*/

//----Include the NodeMCU ESP8266 Library---//
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ESP8266WiFi.h>

//Include the library for the RFID Reader
#include <SPI.h>
#include <MFRC522.h>

//define the pin numbers
#define SS_PIN 2 //--> SDA / SS is connected to pinout D4
#define RST_PIN 5  //--> RST is connected to pinout D3

#define ON_Board_LED 2  //--> Defining an On Board LED, used for indicators when the process of connecting to a wifi router
#define Process_LED 16 // D0 pin for the Process_LED
#define success_LED 4  // D1 pin for the success_LED
#define fail_LED 5

MFRC522 mfrc522(SS_PIN, RST_PIN);  //--> Create MFRC522 instance.

int readsuccess;
byte readcard[4];
char str[32] = "";
String StrUID;

//-----SSID and Password of the access point you want to create from the system-------//
const char* ssid = "AndroidAP_7554";
const char* password = "123456789";

//set the endpoint that data will be dropped
// const String paymentType = "credit";  //change to debit for debitting account
// const String apikey = "somade_daniel";
const String servername = "http://192.168.13.149:3000/attendance";
// const String servername = "http://api.ipify.org";

//add api key and payement type to the endpoint
const String serverApi = servername ;

ESP8266WebServer server(80);  //--> Server on port 80

void setup() {
  Serial.begin(115200); //--> Initialize serial communications with the PC
  
  SPI.begin();      //--> Init SPI bus
  
  mfrc522.PCD_Init(); //--> Init MFRC522 card

  delay(500);
  
  pinMode(ON_Board_LED, OUTPUT);
  pinMode(Process_LED, OUTPUT);
  pinMode(success_LED,OUTPUT);
  pinMode(fail_LED,OUTPUT);
  digitalWrite(ON_Board_LED, LOW); //--> Turn off Led On Board
  digitalWrite(Process_LED, HIGH);
  digitalWrite(success_LED, HIGH);
  digitalWrite(fail_LED, HIGH);
  // WiFi.softAP(ssid, password);  //  -> create the access point
 WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());
  Serial.println("HTTP server started");

  Serial.println("");
  Serial.println("Please tag a card or keychain to see the UID !");
  Serial.println("");
  digitalWrite(Process_LED, LOW);
  digitalWrite(success_LED, LOW);
  digitalWrite(fail_LED, LOW);
}

void loop() {
  readsuccess = getid();
  if (readsuccess) {
    String UIDresultSend, postData; 
    
    // When it reads a card, turn on the LED and Process_LED
    digitalWrite(ON_Board_LED, LOW);
    digitalWrite(Process_LED, HIGH);
    
    // Get the card number and print it on the serial monitor
    UIDresultSend = StrUID;  
    Serial.println(UIDresultSend); 

    // Concatenate the server API URL with the card number to send data + "?card_number=" + UIDresultSend
    String request = serverApi ;
    // WiFi.softAP(ssid, password);
    Serial.print("IP address:\t");
    // Serial.println(WiFi.softAPIP()); 
    Serial.print("Request: "); Serial.println(request);


    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      WiFiClientSecure client;
      WiFiClient no_seq_client;
      http.begin(no_seq_client, request);  // Specify the server URL and WiFi client object
      http.setTimeout(20000);
      client.setInsecure();
      http.addHeader("Content-Type", "application/json");
      String jsonBody = "{\"id\": \"" + StrUID + "\"}";

      int httpCode = http.POST( jsonBody );  // Make the GET request

      // Check the response status code
      if (httpCode > 0 && httpCode<400) {
        Serial.printf("HTTP Response code: %d\n", httpCode);

        // Get the response payload (if any)
        String payload = http.getString();
        Serial.println("Response payload:");
        Serial.println(payload);
        digitalWrite(Process_LED,LOW);
        digitalWrite(success_LED,HIGH);
        delay(1000);
        digitalWrite(success_LED,LOW);
      } else {
        digitalWrite(Process_LED,LOW);
        digitalWrite(fail_LED,HIGH);
        delay(1000);
        digitalWrite(fail_LED,LOW);
        Serial.printf("Error in HTTP request: %s\n", http.errorToString(httpCode).c_str());
      }

      http.end();  // End the connection
  } 
  else {
    Serial.println("WiFi Disconnected");
  }


   
    
    // digitalWrite(ON_Board_LED, LOW);   // Turn on the Process_LED and LED for 0.2 sec after receiving response
    // digitalWrite(Process_LED, HIGH);
    // delay(200);
    // digitalWrite(ON_Board_LED, HIGH);
    // digitalWrite(Process_LED, LOW);

    
    Serial.println("");    // Add a new line in serial monitor
  }
}

//----------------Procedure for reading and obtaining a UID from a card or keychain----------//
int getid() {
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return 0;
  }
  if (!mfrc522.PICC_ReadCardSerial()) {
    return 0;
  }

  Serial.print("THE UID OF THE SCANNED CARD IS : ");

  for (int i = 0; i < 4; i++) {
    readcard[i] = mfrc522.uid.uidByte[i]; //storing the UID of the tag in readcard
    array_to_string(readcard, 4, str);
    StrUID = str;
  }
  mfrc522.PICC_HaltA();
  return 1;
}

//----Procedure to change the result of reading an array UID into a string----------//
void array_to_string(byte array[], unsigned int len, char buffer[]) {
  for (unsigned int i = 0; i < len; i++)
  {
    byte nib1 = (array[i] >> 4) & 0x0F;
    byte nib2 = (array[i] >> 0) & 0x0F;
    buffer[i * 2 + 0] = nib1  < 0xA ? '0' + nib1  : 'A' + nib1  - 0xA;
    buffer[i * 2 + 1] = nib2  < 0xA ? '0' + nib2  : 'A' + nib2  - 0xA;
  }
  buffer[len * 2] = '\0';
}