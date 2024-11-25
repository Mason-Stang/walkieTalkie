#include <SPI.h>     // include Arduino SPI library
#include <SD.h>      // include Arduino SD library
#include <Wire.h>

File root;
File f;

const int MAX_BUF_SIZE = 29;

        // data to be sent
struct I2cTxStruct {
    short numDataBytes;       // 2 bytes
    bool isLastPacket;      // 1
    byte dataBuf[MAX_BUF_SIZE];       // 29
                            //------
                            // 32
};
// struct planning
// unsigned long packetNum // 1 to x  // not needed
// int numDataBytes // 1 to 29
// bool isLastPacket

I2cTxStruct txData;

        // I2C control stuff
const byte thisAddress = 8; // these need to be swapped for the other Arduino
const byte otherAddress = 9;

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
}

void loop() {

  // choose file

  // opening the 552 byte file
  // root.rewindDirectory();
  // for (int i=0; i<2; i++) {
  //   f =  root.openNextFile();
  // }

  // opening all the files sequentially
  f =  root.openNextFile();  // open next file
  if (! f) {
    // no more files
    root.rewindDirectory();  // go to start of the folder
    f = root.openNextFile();
  }

  printFile(f);

  // Send the contents of file f in batches over I2C
  f.seek(0);
  bool lastPacket = false;

  unsigned long numBytesToWrite = f.size();
  Serial.print("File size: ");
  Serial.print(numBytesToWrite, DEC);
  Serial.println();
  // if (numBytesToWrite != f.available()) {
  //   Serial.println("ERROR: Bytes available mismatch: ");
  //   Serial.printf("%d %d", f.size(), f.available());
  //   return;
  // }

  Serial.println("Sending file");

  while (numBytesToWrite > 0) {
    int numBytes = min(numBytesToWrite, MAX_BUF_SIZE);
    if (numBytesToWrite <= MAX_BUF_SIZE) {
      lastPacket = true;
    }

    // Set the struct fields
    txData.numDataBytes = numBytes;
    txData.isLastPacket = lastPacket;
    if (f.read(txData.dataBuf, numBytes) != numBytes) {
      Serial.println("ERROR: failure reading file");
      return;
    }

    // Send
    Wire.beginTransmission(otherAddress);
    Serial.println("Transmission started");
    int numWritten = Wire.write((byte*) &txData, sizeof(txData));
    if (numWritten != sizeof(txData)) {
      Serial.println("ERROR: Failure with Wire.write");
      return;
    }
    Serial.println(numWritten, DEC);
    int ret = Wire.endTransmission();    // this is what actually sends the data
    Serial.println("Transmission ended");
    if (ret != 0) {
      Serial.print("ERROR: error code from Wire.endTransmission: ");
      Serial.println(ret, DEC);
      return;
    }

    numBytesToWrite -= (unsigned long) numBytes;
    delay(100); // idk if a short delay would be helpful?
  }


  Serial.println("All bytes sent!");
  delay(10000); // delay 10 seconds

}

void printFile(File file) {
  if (! file) {
    return;
  }
  Serial.print(file.name());
  Serial.print("\t\t");
  Serial.println(file.size(), DEC);
}