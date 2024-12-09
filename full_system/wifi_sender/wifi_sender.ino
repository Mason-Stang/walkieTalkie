#include <Wire.h>
#include "WiFiS3.h"

/**
 This node forwards packets from recorder to wifi_receiver.
**/

const int MAX_BUF_SIZE = 28;

        // data to be received/sent
struct I2cRxStruct {
    short numDataBytes;       // 2 bytes
    bool hasData;      // 1
    bool wait;         // 1
    byte dataBuf[MAX_BUF_SIZE];       // 28
                            //------
                            // 32
};

volatile I2cRxStruct rxData;
const byte otherAddress = 8;
bool receivingRequest = false;
char ssid[] = "yas" ;
char pass[] = "yasmine2097";
int status = WL_IDLE_STATUS;
WiFiServer server(80);

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // set up I2C
  Wire.begin(); // join i2c bus
  setupWifi();
  Serial.println("Starting system");
}

void loop() {

  //constantly listening for the one byte..
  
  // TODO: execute the following upon receiving a request over wifi from wifi_receiver
  if (receivingRequest) {
    requestData();
    sendData();
    receivingRequest = false;
  } else {
    waitingForRequest();
  }

}

void waitingForRequest(){
  WiFiClient client = server.available(); // will only run if there is data available
  if (client.connected()){
    //waiting for the one byte
    byte buf[1];
    Serial.print(client.read(buf, 1));
    //double check what this is sending
    String ack = String((char *)buf[0]);
    Serial.print(ack);
    // if (ack == "a"){
      //unsure if this was the intention of the boolean but once we recieve the one byte we say that we are 
      //receiving a request from the player.
    receivingRequest = true;
    
    // }
    
  }
}

void setupWifi(){
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
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }
  server.begin();
  // you're connected now, so print out the status:
  printWifiStatus();
  
}

void printPacket() {
    Serial.print("Packet with hasData = ");
    Serial.print(rxData.hasData, DEC);
    Serial.print(" and ");
    Serial.print(rxData.numDataBytes, DEC);
    Serial.println(" data bytes received (bytes in HEX): ");
    for (int i=0; i<rxData.numDataBytes; i++) {
      Serial.print(rxData.dataBuf[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
}


void sendData() {
  // TODO: send rxData to wifi_receiver
  //client.stop();
  //idt this is atomic, might need to disable interrupts.

  printPacket();
  Serial.println("data being sent");
  
  server.write((byte *)&rxData, sizeof(rxData));
  
}

void requestData() {
  //QUESTION: what is the purpose of sending a packet with no data to the receiver/player? 

  int bytesReturned = Wire.requestFrom(otherAddress, sizeof(rxData)); //Note: Blocks for around a second when there's no response
  if (bytesReturned != sizeof(rxData)) {
    rxData.hasData = false;
    rxData.wait = true;
    Serial.print("No data received: ");
    Serial.println(bytesReturned, DEC);
    return;
  }

  while (!Wire.available()); // may not be necessary
  int bytesRead = Wire.readBytes( (byte*) &rxData, sizeof(rxData));
  if (bytesRead != sizeof(rxData)) {
    Serial.print("ERROR: Incorrect number of bytes read: ");
    Serial.println(bytesRead, DEC);
    return;
  } 
  //added this here to say that we have fulffilled the previous request and are ready for the next one
  
}

void printPacket(I2cRxStruct packet) {
    Serial.print("Packet with hasData = ");
    Serial.print(rxData.hasData, DEC);
    Serial.print(" and ");
    Serial.print(rxData.numDataBytes, DEC);
    Serial.println(" data bytes received (bytes in HEX): ");
    for (int i=0; i<rxData.numDataBytes; i++) {
      Serial.print(rxData.dataBuf[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
}


/* -------------------------------------------------------------------------- */
void printWifiStatus() {
/* -------------------------------------------------------------------------- */  
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
}

