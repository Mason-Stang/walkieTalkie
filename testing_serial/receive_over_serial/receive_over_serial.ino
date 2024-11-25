#include <cstdlib>

const int MAX_BUF_SIZE = 50; // at most 64 bytes can fit in serial buffer
const int INITIAL_PACKET_LEN = 6; // 5 digits followed by null terminator
const int DATA_HEADER_LEN = 3; // 2 digits followed by null terminator

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  while (!Serial1);
  Serial1.setTimeout(10);
  Serial.println("Starting system");

}

void loop() {
  static byte buf[MAX_BUF_SIZE];
  static char initialPacket[INITIAL_PACKET_LEN];
  static char dataHeaderPacket[DATA_HEADER_LEN];
  static bool skipFile = false;

  int bytesAvailable = Serial1.available();
  if (bytesAvailable > 0) {
    if (skipFile) { // If initial packet malformed, skip the file by reading bytes until none are available for an iteration of loop.
      char clearBytesBuf[64];
      Serial1.readBytes(clearBytesBuf, bytesAvailable);
      return;
    }

    Serial.println("Receiving file");
    // process incoming packets
    // TODO: reconstruct them into a file

    int initialPacketBytesRead = Serial1.readBytes(initialPacket, INITIAL_PACKET_LEN); 
    if (initialPacket[INITIAL_PACKET_LEN - 1] != '\0') {
      Serial.println("ERROR: initial packet had no terminating character");
      skipFile = true;
      return;
    }
    int expectedPackets = atoi(initialPacket);
    Serial.print("Expected Packets: ");
    Serial.print(expectedPackets);
    Serial.println();

    for (int i = 0; i < expectedPackets; i++) {
      int bytesExpected = -1;
      Serial.println("Receiving data header:");

      int bytesRead = Serial1.readBytes(dataHeaderPacket, DATA_HEADER_LEN);
      if (bytesRead != DATA_HEADER_LEN) {
        Serial.println("ERROR: bytesRead not equal to data header length");
      } else if (dataHeaderPacket[DATA_HEADER_LEN - 1] != '\0') {
        Serial.println("ERROR: data header has no terminating character");
      } else {
        bytesExpected = atoi(dataHeaderPacket);
      }

      if (bytesExpected == -1) { // Replace buffer with null characters if header was malformed and bytesExpected couldn't be set.
        for (int i = 0; i < MAX_BUF_SIZE; i++) {
          buf[i] = '\0';
        }
        // Clear MAX_BUF_SIZE bytes.
        char clearBytesBuf[MAX_BUF_SIZE];
        Serial1.readBytes(clearBytesBuf, MAX_BUF_SIZE);
      } else {
        int bytesRead = Serial1.readBytes(buf, bytesExpected);
      }
      Serial.print("Printing buf (just HEX values): ");
      int bufLen = bytesExpected != -1 ? bytesExpected : MAX_BUF_SIZE;
      for (int i = 0; i < bufLen; i++) {
        Serial.print(buf[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
    }
    Serial.println("Done receiving file");
    // could play the file here
  } else {
    skipFile = false;
  }

}
