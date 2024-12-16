#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <TMRpcm.h>

/**
  This nodes reads in a .wav file from wifi_receiver and plays it.
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

/**
 * Initializes the node.
 * FSM: No effect
 * **/
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

  audio.setVolume(6);    //   0 to 7. Set volume level
  audio.quality(1);      //  Set 1 for 2x oversampling Set 0 for normal

  Serial.println("OK!");
}

/**
 * Loops continuously.
 * Calls requestData(), which requests the next data packet from the wifi_receiver node.
 * When the wifi_receiver node responds with a packet, newRxData=true and the code enters the if statement.
 * FSM:
 * - If receivingFile=false and audio.isPlaying()=false, then currently in the Ready state. 
 *  - If rxData.hasData=true, then we are receiving
 *    the first packet of a file, so transition to the Receive state.
 * - If receivingFile=false and audio.isPlaying()=true, then currently in the Play state, and playing the file. 
 *  - If rxData.hasData=true, then we are receiving
 *    the first packet of a file, so transition to the Ready state, then the Receiving state. Stop playing and start receiving packets.
 * - If receivingFile=true, then currently in the Receive state. 
 *  - If rxData.hasData=true, then we are receiving
 *    the next packet of a file, so stay in the Receive state. 
 *  - If rxData.hasData=false, then we've finished receiving
 *    packets for a file, so transition from Receive state to Play state. Begin playing the file.
 * **/
void loop() {
  if (newRxData) {

    newRxData = false;
    if (rxData.wait) {
      delay(100);
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
      f.write(rxData.dataBuf, rxData.numDataBytes);

      Serial.println("Incoming file...");
      receivingFile = true;

      requestData();
      return;

    } else if (receivingFile && rxData.hasData) {
      f.write(rxData.dataBuf, rxData.numDataBytes);

      requestData();
      return;

    } else if (receivingFile && !rxData.hasData) {
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

/**
 * Sends a request for the next data packet to the wifi_receiver node, and reads the response.
 * Sets global variable rxData to the value of the response.
 * No inputs or outputs.
 * FSM: 
 * If receivingFile=true, currently in Receive state. No change to state.
 * If receivingFile=false, currently in Ready state. No change to state.
 * **/
void requestData() {
  int bytesReturned = Wire.requestFrom(otherAddress, sizeof(rxData));  

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

/**
 * Helper function for printing metadata about the current contents of rxData, 
 * the current data packet.
 * No inputs or outputs.
 * FSM: No effect
 * **/
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
