#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <TMRpcm.h>

/**
This file and node_B are just to test that
requestFrom works in two directions.
Conclusion: It works!
**/

const byte thisAddress = 8; // these need to be swapped for the other Arduino
const byte otherAddress = 9;

char readIn = 'I';
char sendOut = 'A';
volatile bool rqSent = false;

void setup() {
  Serial.begin(115200);
  while (!Serial);

        // set up I2C
  Wire.begin(thisAddress); // join i2c bus
  Wire.onRequest(requestEvent); // register function to be called when a request arrives
  Serial.println("OK!");
}

void loop() {
  requestData();
  Serial.print("Char value (maybe read): ");
  Serial.println(readIn);
  delay(10000);

}

void requestData() {
  byte stop = true;
  byte numBytes = 1;
  int bytesReturned = Wire.requestFrom(otherAddress, numBytes, stop); 

  if (bytesReturned != 1) {
    Serial.print("No data received: ");
    Serial.println(bytesReturned, DEC);
    return;
  }

  while (!Wire.available()); // may not be necessary
  int bytesRead = Wire.readBytes( (byte*) &readIn, 1);
  if (bytesRead != 1) {
    Serial.print("ERROR: Incorrect number of bytes read: ");
    Serial.println(bytesRead, DEC);
    return;
  }
}

void requestEvent() {
    Wire.write((byte*) &sendOut, 1);
}
