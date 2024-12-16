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

bool useWatchdog = true;

/**
 * Initializes the node.
 * FSM: No effect
 * **/
void setup() {
  Serial.begin(9600);
  while (!Serial);

  // set up I2C
  Wire.begin(); // join i2c bus
  setupWifi();
  Serial.println("Starting system");

  if (useWatchdog) {
    initWDT();
    petWDT();
  }
}

/**
 * Loops continuously.
 * When a request for the next data packet is received from wifi_receiver over wifi,
 * requests data from the recorder node, and forwards the data to wifi_receiver.
 * Also pets the watchdog.
 * FSM:
 * When receivingRequest=true, calls sendData() which transitions from Ready to Send state
 * When sendData() returns, we return to this function which means we transition back to Ready state
 * **/
void loop() {

  //constantly listening for the one byte..
  if (receivingRequest) {
    requestData();
    sendData();
    receivingRequest = false;
  } else {
    waitingForRequest();
  }

  if (useWatchdog) {
    petWDT();
  }
}

/**
 * This function listens for communication from the wifi_receiver node. 
 * If it receives a request for data, it sets receivingRequest to true.
 * No inputs or outputs.
 * FSM: In Ready state, no changes to state.
 * **/
void waitingForRequest(){
  WiFiClient client = server.available(); // will only run if there is data available
  if (client.connected()){
    //waiting for the one byte
    byte buf[1];
    Serial.print(client.read(buf, 1));
    //double check what this is sending
    String ack = String((char *)buf[0]);
    Serial.print(ack);
    receivingRequest = true;
  }
}

/**
 * This function establishes the connection to the wifi_receiver node.
 * No inputs or outputs.
 * Executes once on initialization.
 * FSM: No effect
 * **/
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

/**
 * Helper function for printing metadata about the current contents of rxData, 
 * the current data packet.
 * No inputs or outputs.
 * FSM: No effect
 * **/
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

/**
 * Prints the current packet to be sent, and sends the contents of rxData to the wifi_receiver node.
 * No inputs or outputs.
 * FSM: Currently in Send state, no change to state.
 * **/
void sendData() {
  printPacket();
  Serial.println("data being sent");
  server.write((byte *)&rxData, sizeof(rxData));
}

/**
 * Sends a request for the next data packet to the Recorder node, and reads the response.
 * Sets global variable rxData to the value of the response.
 * No inputs or outputs.
 * FSM: In Ready state, no change to state.
 * **/
void requestData() {
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
}


/**
 * Helper function to print the status of the wifi connection.
 * No inputs or outputs.
 * FSM: No effect.
 * **/
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

/**
 * Watchdog function adapted from Lab 4
 * Input: Starting CPU int
 * Output: The next CPU int
 * **/
unsigned int getNextCPUINT(unsigned int start) {
   unsigned int tryInt = start + 1;
      while (tryInt < 32) {
         if (NVIC_GetEnableIRQ((IRQn_Type) tryInt) == 0) {
            return tryInt;
         }
      tryInt++;
   }
}
unsigned int WDT_INT = getNextCPUINT(1);

/**
 * Helper function to initialize the watchdog.
 * No inputs or outputs.
 * Adapted from Lab 4
 * FSM: no effect.
 * **/
void initWDT() {
  R_WDT->WDTCR = 0b0011001110000011;
  R_DEBUG->DBGSTOPCR_b.DBGSTOP_WDT = 0;
  R_WDT->WDTSR = 0; 
  R_WDT->WDTRCR = 1 << 7;
  R_ICU->IELSR[WDT_INT] = 0x025;
  NVIC_SetPriority((IRQn_Type) WDT_INT, 14);
  NVIC_EnableIRQ((IRQn_Type) WDT_INT);
}

/**
 * Calling this function pets the watchdog by resetting the timer.
 * No inputs or outputs.
 * Adapted from Lab 4
 * FSM: no effect.
 * **/
void petWDT() {
  R_WDT->WDTRR = 0x00;
  R_WDT->WDTRR = 0xFF;
}


