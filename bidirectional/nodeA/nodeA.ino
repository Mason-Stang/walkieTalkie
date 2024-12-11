#include "nodeA.h"

/**
Key points about the system:
- Using a design where each node polls the other for data.
  - Doing this over I2C using Wire.requestFrom()
- Bi-directional Architecture:
  nodeA <---(i2c)---> nodeB
- nodeA and nodeB code are exactly the same, except that their addresses are swapped.
**/

// Data
const int MAX_BUF_SIZE = 28;

struct I2cStruct {
    short numDataBytes;       // 2 bytes
    bool hasData;      // 1
    bool wait;         // 1
    byte dataBuf[MAX_BUF_SIZE];       // 28
                            //------
                            // 32
};
I2cStruct rxData;
volatile I2cStruct txData; // volatile because modified by an ISR

const byte THIS_ADDRESS = 8; // these need to be swapped for the other Arduino
const byte OTHER_ADDRESS = 9;

// For receiving
volatile bool receivingFile = false;
volatile bool newRxData = false;
unsigned long pollTimer = 0;

// For sending
volatile bool sendingFile = false;
volatile bool rqSent = true;

// For playing/recording
#define SD_ChipSelectPin 53
TMRpcm audio;
File f_in;
File f_out;
File root;
const int BUTTON_PIN = 2;
const int RECORDING_LED_PIN = 3;
const int MIC_PIN = A0;
const int SAMPLE_RATE = 8000; //16000; // Affects audio quality and file size. Seems ok at 4000 but wiki ways 8000 minimum.
const int SPEAKER_PIN = 11;  // Has to be pin 11 for mega! 9 on Uno
const int VOLUME = 6; // 0 to 7. Set volume level
const int QUALITY = 1; // Set 1 for 2x oversampling Set 0 for normal

volatile bool recording_now = false;
volatile unsigned long time_last_pushed = 0;
bool file_ready = false;

// Files
char out_file[] = "0.wav";
char in_file[] = "1.wav";

#ifdef TESTING
unsigned long numBytesToWrite = 0;
#endif

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.print("Starting...");

  // set up audio recording
  pinMode(MIC_PIN, INPUT);
  pinMode(RECORDING_LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), button_pushed, FALLING);
  audio.CSPin = SD_ChipSelectPin;

  #ifndef TESTING
  // set up SD
  if (!SD.begin(SD_ChipSelectPin)) {
    Serial.println("failed!");
    while(true);
  }
  #endif

  // set up audio playback
  audio.speakerPin = SPEAKER_PIN;
  audio.setVolume(VOLUME);
  audio.quality(QUALITY);

  // set up I2C
  Wire.begin(THIS_ADDRESS); // join i2c bus
  txData.hasData = false; // false when not currently sending a file
  Wire.onRequest(requestEvent); // register function to be called when a request arrives

  Serial.println("OK!");

  #ifdef TESTING
  runTests();
  while (true);
  #endif
}

void loop() {

  if (newRxData) {
    handleNewData();
  }

  if (file_ready) {
    file_ready = false;
    sendingFile = true;
    f_out = SD.open(out_file, FILE_READ);
    f_out.seek(0);
    //printFile();
    Serial.println("Sending file...");
    sendFile();
    f_out.close();
    Serial.println("All bytes sent!");
  }

  if (millis()-pollTimer > 1000) {
    // continuously poll sender every 1 second
    requestData();
    pollTimer = millis();
  }

}

#ifndef TESTING

void handleNewData() {
  newRxData = false;
  if (!receivingFile && rxData.hasData) {
    if (rxData.wait) {
      // shouldn't ever reach this case?
      delay(10);
      requestData();
      return;
    }

    receivingFile = true;
    if (audio.isPlaying()) {
      // Upon receiving a new audio file, stop playing the current one
      audio.stopPlayback();
      Serial.println("Playback stopped.");
    }

    // Make a new file
    if (SD.exists(in_file)) {
      f_in.close();
      SD.remove(in_file);
    }
    f_in = SD.open(in_file, FILE_WRITE);
    if (!f_in) {
      Serial.println("ERROR: file couldn't be opened");
    }
    f_in.write(rxData.dataBuf, rxData.numDataBytes);

    Serial.println("Incoming file...");
    // printPacket(rxData);

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
    f_in.write(rxData.dataBuf, rxData.numDataBytes);

    requestData();
    return;

  } else if (receivingFile && !rxData.hasData) {
    Serial.println("Last packet received. Playing file...");
    Serial.println();
    f_in.flush();
    audio.play(in_file);
    receivingFile = false;

  }
  // else: no file in progress, and sender not sending any data, so do nothing

}

//code below executed each time the button is pressed down
void button_pushed() {
  if (sendingFile || !rqSent || receivingFile) {
    // If we're currently sending a file (sendinfFile), 
    // or that file's last packet hasn't yet been sent (!rqSent),
    // or we're receiving packets for a file (receivingFile)
    // don't respond to button presses.
    return;
  }

  if (audio.isPlaying()) {
    audio.stopPlayback();
    // If audio is currently playing and you press the record button, audio stops playing
  }

  // This fixes button bouncing issue
  if (millis() - time_last_pushed < 500) {
    return;
  }

  if (!recording_now) {
    //isn't recording so starts recording & turns LED on
    recording_now = true;
    digitalWrite(RECORDING_LED_PIN, HIGH);
    audio.startRecording(out_file, SAMPLE_RATE, MIC_PIN);
  }
  else {
    //is recording so stops recording & turns LED off
    recording_now = false;
    digitalWrite(RECORDING_LED_PIN, LOW);
    audio.stopRecording(out_file);
    file_ready = true;
  }
  time_last_pushed = millis();
}

void requestData() {
  byte stop = true;
  int bytesReturned = Wire.requestFrom(OTHER_ADDRESS, sizeof(rxData), stop); 
  //Note: Pauses for around a second when there's no response.
  // But sometimes it blocks??

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

// Send the contents of f_out in packets over I2C
void sendFile() {
  unsigned long numBytesToWrite = f_out.size();
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
      if (f_out.read(txData.dataBuf, numBytes) != numBytes) {
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

#else
/**
Note: Commenting out file-related code for tests, because I don't expect the TAs
running the tests to have an Arduino connected to an SD card module.

Also commenting out some of the blocking serial commands.
**/

void handleNewData() {
  newRxData = false;
  if (!receivingFile && rxData.hasData) {
    if (rxData.wait) {
      // shouldn't ever reach this case?
      delay(10);
      requestData();
      return;
    }

    receivingFile = true;
    if (audio.isPlaying()) {
      // Upon receiving a new audio file, stop playing the current one
      audio.stopPlayback();
      Serial.println("Playback stopped.");
    }

    // Make a new file
    // if (SD.exists(in_file)) {
    //   f_in.close();
    //   SD.remove(in_file);
    // }
    // f_in = SD.open(in_file, FILE_WRITE);
    // if (!f_in) {
    //   Serial.println("ERROR: file couldn't be opened");
    // }
    // f_in.write(rxData.dataBuf, rxData.numDataBytes);

    Serial.println("Incoming file...");
    // printPacket(rxData);

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
    // f_in.write(rxData.dataBuf, rxData.numDataBytes);

    requestData();
    return;

  } else if (receivingFile && !rxData.hasData) {
    Serial.println("Last packet received. Playing file...");
    Serial.println();
    // f_in.flush();
    // audio.play(in_file);
    receivingFile = false;

  }
  // else: no file in progress, and sender not sending any data, so do nothing

}

//code below executed each time the button is pressed down
void button_pushed() {
  if (sendingFile || !rqSent || receivingFile) {
    // If we're currently sending a file (sendinfFile), 
    // or that file's last packet hasn't yet been sent (!rqSent),
    // or we're receiving packets for a file (receivingFile)
    // don't respond to button presses.
    return;
  }

  if (audio.isPlaying()) {
    audio.stopPlayback();
    // If audio is currently playing and you press the record button, audio stops playing
  }

  // This fixes button bouncing issue
  if (millis() - time_last_pushed < 500) {
    return;
  }

  if (!recording_now) {
    //isn't recording so starts recording & turns LED on
    recording_now = true;
    digitalWrite(RECORDING_LED_PIN, HIGH);
    audio.startRecording(out_file, SAMPLE_RATE, MIC_PIN);
  }
  else {
    //is recording so stops recording & turns LED off
    recording_now = false;
    digitalWrite(RECORDING_LED_PIN, LOW);
    audio.stopRecording(out_file);
    file_ready = true;
  }
  time_last_pushed = millis();
}

void requestData() {
  byte stop = true;

  // Note: Commenting this wire request out because it will block when it doesn't receive a response,
  // which would break our tests.

  // int bytesReturned = Wire.requestFrom(OTHER_ADDRESS, sizeof(rxData), stop); 
  int bytesReturned = 0;


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

// Send the contents of f_out in packets over I2C
void sendFile() {
  // unsigned long numBytesToWrite = f_out.size();
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
      // if (f_out.read(txData.dataBuf, numBytes) != numBytes) {
      //   Serial.println("ERROR: failure reading file");
      //   return;
      // }

      numBytesToWrite -= (unsigned long) numBytes;
      rqSent = false;
      if (numBytesToWrite == 0) {
        sendingFile = false;
      }
    }
  }
  // while (!rqSent); // wait for last packet to be sent before setting hasData=false and returning to loop()
  txData.hasData = false;
} 

#endif

void requestEvent() {
    Wire.write((byte*) &txData, sizeof(txData));
    rqSent = true;
    txData.wait = true;
}

// void printPacket(I2cStruct packet) {
//     Serial.print("Packet with hasData = ");
//     Serial.print(rxData.hasData, DEC);
//     Serial.print(" and ");
//     Serial.print(rxData.numDataBytes, DEC);
//     Serial.println(" data bytes received (bytes in HEX): ");
//     for (int i=0; i<rxData.numDataBytes; i++) {
//       Serial.print(rxData.dataBuf[i], HEX);
//       Serial.print(" ");
//     }
//     Serial.println();
// }
