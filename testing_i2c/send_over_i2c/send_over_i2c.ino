#include <SPI.h>     // include Arduino SPI library
#include <SD.h>      // include Arduino SD library
#include <Wire.h>

File root;
File f;

const int MAX_BUF_SIZE = 29;

        // data to be sent
struct I2cTxStruct {
    short numDataBytes;       // 2 bytes
    bool hasData;      // 1
    byte dataBuf[MAX_BUF_SIZE];       // 29
                            //------
                            // 32
};

I2cTxStruct txData;

        // I2C control stuff
const byte thisAddress = 8; // these need to be swapped for the other Arduino
const byte otherAddress = 9;

volatile bool sendingFile = false;
volatile bool rqSent = true;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.print("Initializing SD card...");
  if (!SD.begin(53)) {
    Serial.println("failed!");
    while(true);  // stay here.
  }
  Serial.println("OK!");

  root = SD.open("/");      // open SD card main root
  // f = root.openNextFile();
  // printFile(f);

        // set up I2C
  Wire.begin(thisAddress); // join i2c bus
  txData.hasData = false; // false when not currently sending a file
  Wire.onRequest(requestEvent); // register function to be called when a request arrives
}

void loop() {

    // Option 1: open the 552 byte file
  // root.rewindDirectory();
  // for (int i=0; i<2; i++) {
  //   f =  root.openNextFile();
  // }

    // Option 2: open all the files sequentially
  f =  root.openNextFile();  // open next file
  if (! f) {
    // no more files
    root.rewindDirectory();  // go to start of the folder
    f = root.openNextFile();
  }

  f.seek(0);
  printFile();
  Serial.println("Sending file...");

  sendFile();

  Serial.println("All bytes sent!");
  delay(20000); // delay 10 seconds

}

// Send the contents of file f in batches over I2C
void sendFile() {
  sendingFile = true;
  unsigned long numBytesToWrite = f.size();
  while (sendingFile) {
    // Repeatedly update txData with the next data

    if (rqSent) {
      int numBytes = min(numBytesToWrite, MAX_BUF_SIZE);

      // fill in txData
      txData.numDataBytes = (short) numBytes;
      txData.hasData = true;
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
  txData.hasData = false;
} 


void requestEvent() {
    Wire.write((byte*) &txData, sizeof(txData));
    rqSent = true;
}


void printFile() {
  if (! f) {
    Serial.println("ERROR printing file");
    return;
  }
  Serial.print(f.name());
  Serial.print("\t\t");
  Serial.print("File size: ");
  Serial.println(f.size(), DEC);
}