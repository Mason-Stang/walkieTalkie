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

volatile bool sendingFile = false;
volatile bool rqSent = true;

volatile bool receivingFile = false;
volatile bool newRxData = false;

void setup() {
  Serial.begin(115200);
  while (!Serial);

        // set up I2C
  Wire.begin(thisAddress); // join i2c bus
  txData.hasData = false; // false when not currently sending a file
  Wire.onRequest(requestEvent); // register function to be called when a request arrives
  Serial.println("OK!");
}

void loop() {

  // if (file_ready) {
  //   file_ready = false;
  //   sendingFile = true;
  //   root.rewindDirectory();
  //   f = root.openNextFile();
  //   f.seek(0);
  //   Serial.println("Sending file...");

  //   sendFile();

  //   Serial.println("All bytes sent!");
  // }

  if (newRxData) {
    if (!receivingFile && rxData.hasData) {
      if (rxData.wait) {
        // shouldn't ever reach this case?
        delay(10);
        newRxData = false;
        requestData();
        return;
      }

      Serial.println("Incoming file...");
      // printPacket(rxData);
      receivingFile = true;
      sendingFile = true;
      fillTxData();


    } else if (receivingFile && rxData.hasData) {
      newRxData = false;
      if (rxData.wait) {
        delay(10);
        requestData();
        return;
      }

      // TODO: send consecutive data packets over wifi
      // printPacket(rxData);

      requestData();
      return;

    } else if (receivingFile && !rxData.hasData) {
      // TODO: end the current file transmission over wifi
      Serial.println("Last packet received");
      Serial.println();
      receivingFile = false;
    }

    newRxData = false;
    while (!rqSent); // wait for player to poll for and receive the packet
    requestData();
    return;
  }

  requestData();
  delay(500); // continuously poll sender every 0.5 second

}

void fillTxData() {
  // TODO: Copy rxData into txData.
}

void requestData() {
  // TODO: request data from wifi_sender, populate it into
  // rxData, set newRxData=true if data was received (i.e. wifi_sender was connected to the system)
}

// Send the contents of file f in batches over I2C
void sendFile() {
  unsigned long numBytesToWrite = f.size();
  while (sendingFile) {
    // Repeatedly update txData with the next data

    if (rqSent) {
      int numBytes = min(numBytesToWrite, MAX_BUF_SIZE);
      // Serial.print("numBytes = ");
      // Serial.println(numBytes, DEC);

      // fill in txData
      txData.numDataBytes = (short) numBytes;
      txData.hasData = true;
      txData.wait = false;
      if (f.read(txData.dataBuf, numBytes) != numBytes) {
        Serial.println("ERROR: failure reading file");
        return;
      }

      numBytesToWrite -= (unsigned long) numBytes;
      rqSent = false;
      if (numBytesToWrite == 0) {
        sendingFile = false;
      }
    }
  }
  while (!rqSent); // wait for last packet to be sent before setting hasData=false and returning to loop()
  txData.hasData = false;
} 


void requestEvent() {
    Wire.write((byte*) &txData, sizeof(txData));
    rqSent = true;
    txData.wait = true;
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
