#include <Wire.h>
#include "WiFiS3.h"

/**
  This node forwards packets from wifi_sender to player.
**/

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
const byte thisAddress = 8; // these are swapped for the other Arduino
const byte otherAddress = 9;

volatile bool eventFlag = false;
volatile bool dataReady = false;

bool useWatchdog = true;

/**
 * Initializes the node.
 * FSM: No effect
 * **/
void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.print("Starting...");
  setupWifi();

  // set up I2C
  Wire.begin(thisAddress); // join i2c bus
  Wire.onRequest(requestEvent); // register function to be called when a request arrives
  Serial.println("OK!");

  if (useWatchdog) {
    initWDT();
    petWDT();
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

/**
 * Loops continuously.
 * If eventFlag=true and dataReady=false, calls requestData() which sends a request to 
 * the wifi_sender node for the next data packet.
 * Also pets the watchdog.
 * FSM:
 * Current state is Ready. This function doesn't directly switch states, but it calls
 * requestData which populates rxData, setting dataReady=true. When this is the case, the
 * next data packet is sent to the Player node when requestEvent is invoked.
 * **/
void loop() {
  if (eventFlag && !dataReady) {
    eventFlag = false;
    requestData();

  if (useWatchdog) {
    petWDT();
  }
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

/**
 * sends a request to the wifi_sender node asking for the next data packet.
 * Blocks for 1 second waiting for a response. Reads the response if it arrives, 
 * prints the packet, and sets dataReady=true;
 * No inputs or outputs.
 * FSM:
 * In Ready state, doesn't change the state.
 * **/
void requestData() {
  byte ask[1] = {'a'};
  
  client.write(ask, 1);
  Serial.println("sent ask");
  //waiting for the response
  unsigned long currTime = millis();
  while(!client.available()){
    //if no response after 1 second
    if (millis() - currTime > 1000) {
      return;
    }
  }
  
  byte buf[sizeof(rxData)];
  client.read(buf, sizeof(rxData)); 
  memcpy((byte *) &rxData, buf, sizeof(rxData));
  dataReady = true;
  printPacket();
}

/**
 * This code is executed as an ISR each time the player node requests data.
 * Sends the current contents of rxData to the next node. 
 * No inputs or outputs.
 * FSM:
 * When this function is invoked, state changes from Ready to Send.
 * When this function exits, state changes back from Send to Ready.
 * **/
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

