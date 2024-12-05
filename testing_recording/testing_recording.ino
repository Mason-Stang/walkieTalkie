//Credit to Great Scott for spybug project without which this project would not have been created
//https://www.youtube.com/user/greatscottlab (channel)
//https://youtu.be/7Hn4UFi9wvs (spybug_project)

/*
1. Edit pcmConfig.h
    a: On Uno or non-mega boards, #define buffSize 128. May need to increase.
    b: Uncomment #define ENABLE_RECORDING and #define BLOCK_COUNT 10000UL

2. See https://github.com/TMRh20/TMRpcm/wiki/Advanced-Features#wiki-recording-audio for
   additional informaiton.
*/

//adds necessary libraries
#include <SD.h>
#include <SPI.h>
#include <TMRpcm.h>

//creates global variables
#define SD_ChipSelectPin 53 //10
TMRpcm audio;
volatile int file_number = 0;
volatile bool recording_now = false;
const int button_pin = 2;
const int recording_led_pin = 3;
const int mic_pin = A0;
const int sample_rate = 4000; //16000; // Affects audio quality and file size. Seems ok at 4000.
volatile unsigned long time_last_pushed = 0;


//code below executed each time the button is pressed down
void button_pushed() {
  // trying to fix button bouncing
  if (millis() - time_last_pushed < 500) {
    return;
  }

  //combines the neccessary info to find the get a char array of the file name
  char file_name[20] = "";
  itoa(file_number,file_name,10);
  strcat(file_name,".wav");

  if (!recording_now) {
    //isn't recording so starts recording & turns LED on
    recording_now = true;
    digitalWrite(recording_led_pin, HIGH);
    audio.startRecording(file_name, sample_rate, mic_pin);
    Serial.println(file_name);
  }
  else {
    //is recording so stops recording & turns LED off
    recording_now = false;
    digitalWrite(recording_led_pin, LOW);
    audio.stopRecording(file_name);
    file_number++;
  }
  time_last_pushed = millis();
}

void setup() {
  //initialises the serial connection between the arduino and any connected serial device(e.g. computer, phone, raspberry pi...)
  Serial.begin(115200);
  Serial.println("loading...");

  //Sets up the pins
  pinMode(mic_pin, INPUT);
  pinMode(recording_led_pin, OUTPUT);
  pinMode(button_pin, INPUT_PULLUP);

  //Sets up the audio recording functionality
  attachInterrupt(digitalPinToInterrupt(button_pin), button_pushed, FALLING);
  SD.begin(SD_ChipSelectPin);
  audio.CSPin = SD_ChipSelectPin;
}

void loop() {
  //doesn't loop through any code. As we have attached an interupt the arduino will notify our code when the button_pin is FALLING(when pressed down)
}
