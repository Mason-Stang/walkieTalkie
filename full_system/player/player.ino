#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <TMRpcm.h>

/**
  This nodes reads in a .wav file and plays it.
**/

const int MAX_BUF_SIZE = 28;

        // data to be sent
struct I2cRxStruct {
    short numDataBytes;       // 2 bytes
    bool hasData;      // 1
    bool wait;         // 1
    byte dataBuf[MAX_BUF_SIZE];       // 28
                            //------
                            // 32
};

I2cRxStruct rxData;

const byte otherAddress = 8;
const byte thisAddress = 9;

volatile bool receivingFile = false;
volatile bool newRxData = false;

TMRpcm audio;
File f;
char file_name[20] = "";

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // set up I2C
  //Wire.begin(thisAddress); // join i2c bus
  Wire.begin(); // join i2c bus

  Serial.print("Initializing SD card...");
  if (!SD.begin(53)) {
    Serial.println("failed!");
    while(true);  // stay here.
  }
  strcat(file_name,"0.wav");

  audio.speakerPin = 11;  // Has to be pin 11 for mega! 9 on Uno

  //root = SD.open("/");      // open SD card main root
  audio.setVolume(6);    //   0 to 7. Set volume level
  audio.quality(1);      //  Set 1 for 2x oversampling Set 0 for normal

  //Wire.onReceive(receiveEvent);

  Serial.println("OK!");
}

void loop() {

  if (newRxData) {
    //Serial.println("newRxData");
    printPacket();

    newRxData = false;
    
    if (rxData.wait) {
      Serial.println("Waiting for data");
      delay(50);
      requestData();
      return;

    } else if (!receivingFile && rxData.hasData) {
      if (audio.isPlaying()) {
        // Upon receiving a new audio file, stop playing the current one
        audio.stopPlayback();
        Serial.println("Playback stopped.");
      }

      // Make a new file
      if (SD.exists(file_name)) {
        f.close();
        SD.remove(file_name);
      }
      f = SD.open(file_name, FILE_WRITE);
      if (!f) {
        Serial.println("ERROR: file couldn't be opened");
      }
      //file_open = true;
      f.write(rxData.dataBuf, rxData.numDataBytes);

      Serial.println("Incoming file...");
      // printPacket(rxData);
      receivingFile = true;

      requestData();
      return;

    } else if (receivingFile && rxData.hasData) {
      Serial.println("Data packets being received");

      // Append more data to the file
      // printPacket(rxData);
      f.write(rxData.dataBuf, rxData.numDataBytes);

      requestData();
      return;

    } else if (receivingFile && !rxData.hasData) {
      // TODO: close the current file
      Serial.println("Last packet received. Playing file...");
      Serial.println();
      receivingFile = false;
      f.flush();
      audio.play(file_name);

    } else {
      Serial.println("Receiving empty packet");
    }
    // else: no file in progress, and sender not sending any data, so do nothing

  }

  delay(500); // continuously poll sender every 0.5 second
  requestData();

}

void requestData() {
  //Serial.println("Data requested");
  int bytesReturned = Wire.requestFrom(otherAddress, sizeof(rxData));
  //Note: Pauses for around a second when there's no response.
  // But sometimes it blocks??
  

  if (bytesReturned != sizeof(rxData)) {
    Serial.print("No data received: ");
    Serial.println(bytesReturned, DEC);
    return;
  }

  //Serial.println("Before check");
  while (!Wire.available()); // may not be necessary
  //Serial.println("After check");

  int bytesRead = Wire.readBytes( (byte*) &rxData, sizeof(rxData));
  if (bytesRead != sizeof(rxData)) {
    Serial.print("ERROR: Incorrect number of bytes read: ");
    Serial.println(bytesRead, DEC);
    return;
  }
  newRxData = true;
  //Serial.println("Bytes returned");

  // ------------------

  // char x = 'A';
  // newRxData = false; // unnecessary, should already be false
  // Wire.beginTransmission(otherAddress);
  // Wire.write(x);
  // Wire.endTransmission();    // this is what actually sends the data

  // unsigned long currTime = millis();
  // while (!newRxData) {
  //   // block for up to 1 second waiting for the response
  //   // after that, expects that it won't come (undefined behavior if it does)
  //   if (millis() - currTime > 1000) {
  //     // assume the response is never coming
  //     Serial.println("No response from wifi_receiver");
  //     return;
  //   }
  // }
  // Serial.println("Data received");

}

// void receiveEvent() {
//   Wire.readBytes( (byte*) &rxData, sizeof(rxData));
//   newRxData = true;
// }

void printPacket() {
  Serial.print("Packet with hasData = ");
  Serial.print(rxData.hasData, DEC);
  Serial.print(" Packet with wait = ");
  Serial.print(rxData.wait, DEC);
  Serial.print(" and ");
  Serial.print(rxData.numDataBytes, DEC);
  Serial.println(" data bytes received (bytes in HEX): ");
  for (int i=0; i<rxData.numDataBytes; i++) {
    Serial.print(rxData.dataBuf[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}
