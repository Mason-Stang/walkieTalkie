#include <Wire.h>

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

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // set up I2C
  Wire.begin(); // join i2c bus

  Serial.println("Starting system");
}

void loop() {

  // TODO: execute the following upon receiving a request over wifi from wifi_receiver
  if (receivingRequest) {
    requestData();
    sendData();
  }
  

}

void sendData() {
  // TODO: send rxData to wifi_receiver
}

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
