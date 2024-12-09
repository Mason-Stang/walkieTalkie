#include <Wire.h>
#include "WiFiS3.h"

/**
  This node forwards packets from wifi_sender to player.
**/

// when waiting on response, set hasData=false, wait=true
// Keep a variable for when packet is sent. requestData() sets it to true, requestEvent sets it to false if it was true

const int MAX_BUF_SIZE = 28;
char ssid[] = "yas" ;
char pass[] = "yasmine2097";
IPAddress server(172, 20, 10, 12);
int status = WL_IDLE_STATUS;
WiFiClient client;
unsigned long lastConnectionTime = 0;            // last time you connected to the server, in milliseconds
const unsigned long postingInterval = 10L * 1000L; // delay between updates, in milliseconds



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

        // I2C control stuff
const byte thisAddress = 8; // these need to be swapped for the other Arduino
const byte otherAddress = 9;

volatile bool eventFlag = false;
// volatile bool errorFlag1 = false;
// volatile bool errorFlag2 = false;
volatile bool dataReady = false;

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.print("Starting...");
  setupWifi();    //commented out for testing

  // set up I2C
  Wire.begin(thisAddress); // join i2c bus
  // Wire.onReceive(receiveEvent); // register function to be called when a request arrives
  Wire.onRequest(requestEvent); // register function to be called when a request arrives
  Serial.println("OK!");
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
    status = WiFi.begin(ssid,pass);

    // wait 10 seconds for connection:
    delay(10000);
  }

  // you're connected now, so print out the status:
  printWifiStatus();
  bool connected = false;
  while(!connected){
    connected = client.connect(server, 80);
    Serial.println(connected);
  }
}
void loop() {
  if (eventFlag && !dataReady) {
    eventFlag = false;
    requestData();

    // delay(100);
    // Wire.beginTransmission(otherAddress);
    // Wire.write((byte*) &rxData, sizeof(rxData));
    // Wire.endTransmission();    // this is what actually sends the data
    // Serial.println("Bytes written");
  }
  // if (errorFlag1) {
  //   errorFlag1 = false;
  //   Serial.println("Error: numBytesRead != 1");
  // }
  // if (errorFlag2) {
  //   errorFlag2 = false;
  //   Serial.println("Error: byte read != A");
  // }
}

void printPacket() {
  Serial.print("Packet with hasData = ");
  Serial.print(rxData.hasData, DEC);
  Serial.print(" Packet with wait = ");
  Serial.print(rxData.wait, DEC);
  Serial.print(" and ");
  Serial.print(rxData.numDataBytes, DEC);
  Serial.println(" data bytes received (bytes in HEX): ");
  for (int i=0; i<rxData.numDataBytes; i++) {
    Serial.print(rxData.dataBuf[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

void requestData() {
  // TODO: send a request to wifi_sender, wait for a response.
  // Populate response data into rxData
  // If no response, set rxData.wait=true and rxData.hasData=false

  //sends a request to wifi_sender
  byte ask[1] = {'a'};
  
  client.write(ask, 1);
  Serial.println("sent ask");
  //waiting for the response
  unsigned long currTime = millis();
  while(!client.available()){
    //if no response after 0.5 seconds, set rxData.wait=true and rxData.hasData=false, and return;
    if (millis() - currTime > 1000) {
      return;
    }
  }
  // we know there is a client and data to read after we are done waiting. insert watchdog somewhere here?
  //Serial.println("received data");
  
  byte buf[sizeof(rxData)];
  client.read(buf, sizeof(rxData)); //currently on reading the data buffer.
  //noInterrupts();
  memcpy((byte *) &rxData, buf, sizeof(rxData));
  dataReady = true;
  //interrupts();
  printPacket();
}

void requestEvent() {
  eventFlag = true;
  if (dataReady) {
    dataReady = false;
  } else {
    rxData.hasData = false;
    rxData.wait = true;
  }
  Wire.write((byte*) &rxData, sizeof(rxData));
}


// void receiveEvent(int numBytesReceived) {
//   if (numBytesReceived != 1) {
//     errorFlag1 = true;
//     return;
//   }
//   char c = Wire.read();
//   if (c != 'A') {
//     errorFlag2 = true;
//     return;
//   }
//   eventFlag = true;
// }



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

