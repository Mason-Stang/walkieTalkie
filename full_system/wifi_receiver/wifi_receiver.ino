#include <Wire.h>

/**
  This node forwards packets from wifi_sender to player.
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

        // I2C control stuff
const byte thisAddress = 8; // these need to be swapped for the other Arduino
const byte otherAddress = 9;

void setup() {
  Serial.begin(115200);
  while (!Serial);

        // set up I2C
  Wire.begin(thisAddress); // join i2c bus
  Wire.onRequest(requestEvent); // register function to be called when a request arrives
  Serial.println("OK!");
}

void loop() {

}

void requestData() {
  // TODO: send a request to wifi_sender, wait for a response.
  // Populate response data into rxData
  // If no response, set rxData.wait=true and rxData.hasData=false
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
