#include <SPI.h>     // include Arduino SPI library
#include <SD.h>      // include Arduino SD library

File root;
File f;

int MAX_BUF_SIZE = 50; // at most 64 bytes can fit in serial buffer

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial1.begin(9600);
  while (!Serial1);

  Serial.print("Initializing SD card...");
  if (!SD.begin(53)) {
    Serial.println("failed!");
    while(true);  // stay here.
  }
  Serial.println("OK!");

  root = SD.open("/");      // open SD card main root
  f = root.openNextFile();
  printFile(f);

}

void loop() {

  // Send the contents of file f in batches over Serial1
  Serial.println("Sending file");
  f.seek(0);
  static byte buf[MAX_BUF_SIZE];
  //int bufLen;
  static char str[5];
  static int strLen;
  bool lastPacket = false;

  int numBytesToWrite = (int) f.size();
  if (numBytesToWrite != f.available()) {
    Serial.println("ERROR: Bytes available mismatch");
    return;
  }

  while (numBytesToWrite > 0) {
    int numBytes = min(numBytesToWrite, MAX_BUF_SIZE);
    if (numBytesToWrite <= MAX_BUF_SIZE {
      lastPacket = true;
    }

    // first send string message with number of bytes that will be sent
    strLen = sprintf(str, "%d %02d\n", lastPacket ? 1 : 0, numBytes);
    if (strLen != 5) {
      Serial.println("ERROR: Invalid strLen value");
      return;
    }
    Serial1.write(str, strLen);

    // next, send the bytes
    if (f.read(buf, numBytes) != numBytes) {
      Serial.println("ERROR: failure reading file");
      return;
    }
    Serial1.write(buf, numBytes);

    numBytesToWrite -= numBytes;
    delay(10); // idk if a short delay would be helpful?
  }


  Serial.println("All bytes sent!")
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