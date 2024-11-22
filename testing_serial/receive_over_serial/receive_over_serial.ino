

int MAX_BUF_SIZE = 50; // at most 64 bytes can fit in serial buffer

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial1.begin(9600);
  while (!Serial1);
  Serial1.setTimeout(10);
  Serial.println("Starting system");

}

void loop() {
  static byte buf[MAX_BUF_SIZE];
  static char str[5];
  
  if (Serial1.available() > 0) {
    Serial.println("Receiving file");
    // process incoming packets
    // TODO: reconstruct them into a file

    bool isDataMsg = false;
    bool lastPacketArrived = false;
    int bytesExpected = -1;
    while (true) {
      if (!isDataMsg) {
        Serial.println("Receiving non-data message:");
        int bytesRead = Serial1.readBytes(str, 5);
        if (bytesRead != 5) {
          Serial.println("ERROR: bytesRead not equal to 5");
          return;
        }
        if (str[4] != '\n') {
          Serial.println("ERROR: no terminating character");
          return;
        }
        // update lastPacketArrived
        if (str[0] == '1') {
          lastPacketArrived = true;
          // just need to read one more data packet
        }
        // update bytesExpected
        if (!(isDigit(str[2]) && isDigit(str[3]))) {
          Serial.println("ERROR: invalid formatting of bytes expected");
          return;
        }
        bytesExpected = 0;
        bytesExpected += (str[2] - '0') * 10;
        bytesExpected += (str[3] - '0');

        Serial.print("Printing str: ");
        Serial.println(str);

      } else {
        Serial.println("Receiving data message");
        if (bytesExpected == -1) {
          Serial.println("ERROR: bytesExpected never updated");
          return;
        }
        int bytesRead = Serial1.readBytes(buf, bytesExpected);
        if (bytesRead != bytesExpected) {
          Serial.println("ERROR: bytesRead not equal to bytesExpected");
          return;
        }
        Serial.print("Printing buf (just HEX values): ");
        for (int i=0; i<bytesRead; i++) {
          Serial.print(buf[i], HEX);
          Serial.print(" ");
        }
        Serial.println();
        
        bytesExpected = -1;
        if (lastPacketArrived) {
          // done reading the last data packet
          break;
        }
      }

      isDataMsg = !isDataMsg; // expect sender to alternate

    }

    Serial.println("Done receiving file");
    // could play the file here
  }

}
