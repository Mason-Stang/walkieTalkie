#include <Wire.h>
#include "WiFiS3.h"

/**
  This node forwards packets from wifi_sender to player.
**/

const int MAX_BUF_SIZE = 28;
char ssid[] = "Brown-Guest" ;
WiFiServer server(80);
int status = WL_IDLE_STATUS;

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

void setup() {
  Serial.begin(115200);
  while (!Serial);

        // set up I2C
  Wire.begin(thisAddress); // join i2c bus
  Wire.onRequest(requestEvent); // register function to be called when a request arrives
  setupWifi();
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
    status = WiFi.begin(ssid);

    // wait 10 seconds for connection:
    delay(10000);
  }
  server.begin();
  // you're connected now, so print out the status:
  printWifiStatus();
}
void loop() {

}

void requestData() {
  // TODO: send a request to wifi_sender, wait for a response.
  // Populate response data into rxData
  // If no response, set rxData.wait=true and rxData.hasData=false

  //sends a request to wifi_sender
  WiFiClient client = server.available();
  byte buf[1] = {'a'};
  if(client){
    server.write(buf, 1);
  }
  //waiting for the response
  while(!server.available()){

  }
  // we know there is a client and data to read after we are done waiting. insert watchdog somewhere here?
  if (client.connected()) {
    byte buf[MAX_BUF_SIZE];
    int size = client.read(buf, MAX_BUF_SIZE); //currently on reading the data buffer.
    byte result[size];
    memcpy(result, buf, size);

    rxData.dataBuf = result;
    rxData.hasData = true;
    rxData.wait = false;
    rxData.numDataBytes = size;
  }
  //not sure how the hasdata and wait parts of the struct integrate here because we can busy-wait/block until there is data available 
  //for the recorder which is what this code does. feel free to change.

}


void requestEvent() {
  requestData();
  Wire.write((byte*) &rxData, sizeof(rxData));
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
