#include <Wire.h>
#include "WiFiS3.h"

/**
  This node forwards packets from wifi_sender to player.
**/

const int MAX_BUF_SIZE = 28;
char ssid[] = "Brown-Guest" ;
int status = WL_IDLE_STATUS;
WiFiClient client;
IPAddress server(172, 18, 129, 55); //put in the server arduino ip address
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
bool flag = false;
bool flag1 = false;
void setup() {
  Serial.begin(115200);
  while (!Serial);
  setupWifi();
  // set up I2C
  Wire.begin(thisAddress); // join i2c bus
  Wire.onRequest(requestEvent); // register function to be called when a request arrives
  rxData.hasData = false;
  rxData.wait = true;
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

  // you're connected now, so print out the status:
  printWifiStatus();
  bool connected = false;
  while(!connected){
    connected = client.connect(server, 80);
    Serial.println(connected);
  }
}
void loop() {
  if (flag1){
    requestData();
    Wire.write((byte*) &rxData, sizeof(rxData));
    flag1 = false;
    rxData.hasData = false;
    rxData.wait = true;
  }
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
    if (millis() - currTime > 500) {
      rxData.wait = true;
      rxData.hasData = false;
      return;
    }
    currTime = millis();
  }
  // we know there is a client and data to read after we are done waiting. insert watchdog somewhere here?
  Serial.println("recieved data");
  
  byte buf[sizeof(rxData)];
  client.read(buf, sizeof(rxData)); //currently on reading the data buffer.
  memcpy((byte *) &rxData, buf, sizeof(rxData));
  printPacket();
}


void requestEvent() {
  flag1=true;
  //requestData();
  //Wire.write((byte*) &rxData, sizeof(rxData));
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

