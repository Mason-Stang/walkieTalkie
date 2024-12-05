#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <TMRpcm.h>

/**
Notes:
- Start up receiver before starting up sender
**/

// For sending
File root;
File f;

const int MAX_BUF_SIZE = 28;

        // data to be sent
struct I2cTxStruct {
    short numDataBytes;       // 2 bytes
    bool hasData;      // 1
    bool wait;         // 1
    byte dataBuf[MAX_BUF_SIZE];       // 28
                            //------
                            // 32
};

volatile I2cTxStruct txData;

        // I2C control stuff
const byte thisAddress = 8; // these need to be swapped for the other Arduino
const byte otherAddress = 9;

volatile bool sendingFile = false;
volatile bool rqSent = true;

// For recording
#define SD_ChipSelectPin 53 //10
TMRpcm audio;
volatile bool recording_now = false;
const int button_pin = 2;
const int recording_led_pin = 3;
const int mic_pin = A0;
const int sample_rate = 4000; //16000; // Affects audio quality and file size. Seems ok at 4000.
volatile unsigned long time_last_pushed = 0;
bool file_ready = false;


//code below executed each time the button is pressed down
void button_pushed() {
  if (sendingFile || !rqSent) {
    // If we're currently sending a file (sendinfFile), 
    // or that file's last packet hasn't yet been sent (!rqSent),
    // don't respond to button presses.
    return;
  }

  // This fixes button bouncing issue
  if (millis() - time_last_pushed < 500) {
    return;
  }

  char file_name[20] = "";
  strcat(file_name,"0.wav");

  if (!recording_now) {
    //isn't recording so starts recording & turns LED on
    recording_now = true;
    digitalWrite(recording_led_pin, HIGH);
    audio.startRecording(file_name, sample_rate, mic_pin);
  }
  else {
    //is recording so stops recording & turns LED off
    recording_now = false;
    digitalWrite(recording_led_pin, LOW);
    audio.stopRecording(file_name);
    file_ready = true;
  }
  time_last_pushed = millis();
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.print("Initializing SD card...");
  if (!SD.begin(53)) {
    Serial.println("failed!");
    while(true);  // stay here.
  }

  //Sets up the pins
  pinMode(mic_pin, INPUT);
  pinMode(recording_led_pin, OUTPUT);
  pinMode(button_pin, INPUT_PULLUP);

  //Sets up the audio recording functionality
  attachInterrupt(digitalPinToInterrupt(button_pin), button_pushed, FALLING);
  SD.begin(SD_ChipSelectPin);
  audio.CSPin = SD_ChipSelectPin;

  root = SD.open("/");      // open SD card main root

        // set up I2C
  Wire.begin(thisAddress); // join i2c bus
  txData.hasData = false; // false when not currently sending a file
  Wire.onRequest(requestEvent); // register function to be called when a request arrives
  Serial.println("OK!");
}

void loop() {

  if (file_ready) {
    file_ready = false;
    sendingFile = true;
    root.rewindDirectory();
    f = root.openNextFile();
    f.seek(0);
    printFile();
    Serial.println("Sending file...");

    sendFile();

    Serial.println("All bytes sent!");
  }

}

// Send the contents of file f in batches over I2C
void sendFile() {
  unsigned long numBytesToWrite = f.size();
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
      if (f.read(txData.dataBuf, numBytes) != numBytes) {
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


void requestEvent() {
    Wire.write((byte*) &txData, sizeof(txData));
    rqSent = true;
    txData.wait = true;
}


void printFile() {
  if (! f) {
    Serial.println("ERROR printing file");
    return;
  }
  Serial.print(f.name());
  Serial.print("\t\t");
  Serial.print("File size: ");
  Serial.println(f.size(), DEC);
}