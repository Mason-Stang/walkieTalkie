#include <Wire.h>

const int MAX_BUF_SIZE = 29;

        // data to be sent
struct I2cRxStruct {
    int numDataBytes;       // 2 bytes
    bool isLastPacket;      // 1
    byte dataBuf[MAX_BUF_SIZE];       // 29
                            //------
                            // 32
};

I2cRxStruct rxData;

const byte thisAddress = 9; // these need to be swapped for the other Arduino
const byte otherAddress = 8;

bool newFile = true;
volatile bool newRxData = false;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // set up I2C
  Wire.begin(thisAddress); // join i2c bus
  Wire.onReceive(receiveEvent); // register event

  Serial.println("Starting system");
}

/**
Potential problem: The ISR (receiveEvent) will be called with new incoming data before loop can process the previous batch,
causing the ISR to drop the incoming data.

**/

void loop() {

  // For now, printing each packet.
  if (newRxData) {
    if (newFile) {
      Serial.println("Incoming file...");
      newFile = false;
    }

    Serial.print("Packet with ");
    Serial.print(rxData.numDataBytes, DEC);
    Serial.println(" bytes received (in HEX): ");
    for (int i=0; i<rxData.numDataBytes; i++) {
      Serial.print(rxData.dataBuf[i], HEX);
      Serial.print(" ");
    }
    Serial.println();

    if (rxData.isLastPacket) {
      Serial.println("Last packet received");
      Serial.println();
      newFile = true;
    }

    newRxData = false;
  }

}

  // this function is called by the Wire library when a message is received
void receiveEvent(int numBytesReceived) {

  if (numBytesReceived != sizeof(rxData)) {
    Serial.println("ERROR: Incorrect number of bytes received");
    return;
  }

  if (newRxData == false) {
      // copy the data to rxData
    Wire.readBytes( (byte*) &rxData, numBytesReceived);
    newRxData = true;
  } else {
      // loop hasn't processed the previous packet
    Serial.println("ERROR: Data dumped");
      // dump the data
    while(Wire.available() > 0) {
        byte c = Wire.read();
    }
  }
}
