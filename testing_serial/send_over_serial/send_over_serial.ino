#include <SPI.h>     // include Arduino SPI library
#include <SD.h>      // include Arduino SD library

File root;
File f;

const int MAX_BUF_SIZE = 50; // at most 64 bytes can fit in serial buffer
const int INITIAL_PACKET_LEN = 6; // 5 digits followed by null terminator
const int DATA_HEADER_LEN = 3; // 2 digits followed by null terminator

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
  //f = root.openNextFile();
  //printFile(f);

}

void loop() {

  // Send the contents of file f in batches over Serial1
  Serial.println("Sending file");
  f =  root.openNextFile();  // open next file
  if (! f) {
    // no more files
    root.rewindDirectory();  // go to start of the folder
    f = root.openNextFile();
  }
  f.seek(0);
  printFile(f);
  static byte buf[MAX_BUF_SIZE];
  static char initialPacket[INITIAL_PACKET_LEN];
  static char dataHeaderPacket[DATA_HEADER_LEN];
  static int strLen;

  unsigned long numBytesToWrite = f.size();
  int numPackets = (numBytesToWrite + MAX_BUF_SIZE - 1) / MAX_BUF_SIZE;
  strLen = sprintf(initialPacket, "%05d", numPackets);
  // if (strLen != INITIAL_PACKET_LEN) {
  //   Serial.println("ERROR: Invalid initial packet length");
  //   Serial.println(strLen, DEC);
  //   Serial.println(initialPacket[5] == '\0');
  //   return;
  // }
  Serial1.write(initialPacket, strLen+1);

  while (numBytesToWrite > 0) {
    int numBytes = min(numBytesToWrite, MAX_BUF_SIZE);

    // first send string message with number of bytes that will be sent
    strLen = sprintf(dataHeaderPacket, "%02d", numBytes);
    // if (strLen != DATA_HEADER_LEN) {
    //   Serial.println("ERROR: Invalid data header length");
    //   return;
    // }
    Serial1.write(dataHeaderPacket, strLen+1);

    // next, send the bytes
    if (f.read(buf, numBytes) != numBytes) {
      Serial.println("ERROR: failure reading file");
      return;
    }
    Serial1.write(buf, numBytes);

    numBytesToWrite -= numBytes;
    delay(10); // idk if a short delay would be helpful?
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