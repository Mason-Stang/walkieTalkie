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

volatile bool receivingFile = false;
volatile bool newRxData = false;
//bool file_open = false;

TMRpcm audio;
//File root;
File f;
char file_name[20] = "";

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // set up I2C
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

  Serial.println("OK!");
}

void loop() {

  if (newRxData) {
    newRxData = false;
    if (!receivingFile && rxData.hasData) {
      if (rxData.wait) {
        // shouldn't ever reach this case?
        delay(10);
        requestData();
        return;
      }

      if (audio.isPlaying()) {
        // Upon receiving a new audio file, stop playing the current one
        audio.stopPlayback();
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
      if (rxData.wait) {
        delay(10);
        requestData();
        return;
      }

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

    }
    // else: no file in progress, and sender not sending any data, so do nothing

  }

  // if (!audio.isPlaying()) {
  //   if (file_open) {
  //     f.close();
  //     file_open = false;
  //   }
  //   requestData();
  //   delay(500); // continuously poll sender every 0.5 second
  // }
  requestData();
  delay(500); // continuously poll sender every 0.5 second

}

void requestData() {
  int bytesReturned = Wire.requestFrom(otherAddress, sizeof(rxData)); //Note: Pauses for around a second when there's no response
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
