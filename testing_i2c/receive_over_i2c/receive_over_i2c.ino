#include <Wire.h>

const int MAX_BUF_SIZE = 29;

        // data to be sent
struct I2cRxStruct {
    short numDataBytes;       // 2 bytes
    bool hasData;      // 1
    byte dataBuf[MAX_BUF_SIZE];       // 29
                            //------
                            // 32
};

I2cRxStruct rxData;

const byte thisAddress = 9; // these need to be swapped for the other Arduino
const byte otherAddress = 8;

volatile bool receivingFile = false;
volatile bool newRxData = false;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // set up I2C
  Wire.begin(thisAddress); // join i2c bus

  Serial.println("Starting system");
}

void loop() {

  // For now, just printing each packet.
  if (newRxData) {
    if (!receivingFile && rxData.hasData) {
      // TODO: make a new file
      Serial.println("Incoming file...");
      printPacket(rxData);
      receivingFile = true;

      newRxData = false;
      delay(50); // (maybe not needed) wait for sender to refill txData
      requestData();
      return;

    } else if (receivingFile && rxData.hasData) {
      // TODO: append more data to the file
      printPacket(rxData);

      newRxData = false;
      delay(50); // (maybe not needed) wait for sender to refill txData
      requestData();
      return;

    } else if (receivingFile && !rxData.hasData) {
      // TODO: close the current file
      Serial.println("Last packet received");
      Serial.println();
      receivingFile = false;
      delay(50); // (maybe not needed) wait for sender to refill txData 
    }
    // else: no file in progress, and sender not sending any data, so do nothing

    newRxData = false;
  }

  requestData();
  delay(1000); // continuously poll sender every 1 second

}

void requestData() {
  byte stop = false;
  int bytesReturned = Wire.requestFrom(otherAddress, sizeof(rxData), stop);
  if (bytesReturned != sizeof(rxData)) {
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
  newRxData = true;
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
