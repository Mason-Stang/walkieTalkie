//===================
// Using I2C to send and receive structs between two Arduinos
//   SDA is the data connection and SCL is the clock connection
//   On an Uno  SDA is A4 and SCL is A5
//   On an Mega SDA is 20 and SCL is 21
//   GNDs must also be connected
//===================


        // data to be sent and received
// struct I2cTxStruct {
//     char textA[16];         // 16 bytes
//     int valA;               //  2
//     unsigned long valB;     //  4
//     byte padding[10];       // 10
//                             //------
//                             // 32
// };

// struct I2cRxStruct {
//     char textB[16];         // 16 bytes
//     int valC;               //  2
//     unsigned long valD;     //  4
//     byte padding[10];       // 10
//                             //------
//                             // 32
// };

// I2cTxStruct txData = {"xxx", 236, 0};
// I2cRxStruct rxData;
volatile char txData = 'A';
volatile char rxData;

volatile bool newTxData = false;
volatile bool newRxData = false;


        // I2C control stuff
#include <Wire.h>

const byte thisAddress = 9; // these need to be swapped for the other Arduino
const byte otherAddress = 8;


        // timing variables
unsigned long prevUpdateTime = 0;
unsigned long updateInterval = 500;

//=================================

void setup() {
    Serial.begin(115200);
    Serial.println("\nStarting I2C SwapRoles demo\n");

        // set up I2C
    Wire.begin(thisAddress); // join i2c bus
    Wire.onReceive(receiveEvent); // register function to be called when a message arrives

}

//============

void loop() {

        // this bit checks if a message has been received
    if (newRxData == true) {
        showNewData();
        newRxData = false;
    }


        // this function updates the data in txData
    updateDataToSend();
        // this function sends the data if one is ready to be sent
    transmitData();
}

//============

void updateDataToSend() {

    if (millis() - prevUpdateTime >= updateInterval) {
        prevUpdateTime = millis();
        if (newTxData == false) { // ensure previous message has been sent

            // char sText[] = "SendA";
            // strcpy(txData.textA, sText);
            // txData.valA += 10;
            // if (txData.valA > 300) {
            //     txData.valA = 236;
            // }
            // txData.valB = millis();
            newTxData = true;
        }
    }
}

//============

void transmitData() {

    if (newTxData == true) {
        Wire.beginTransmission(otherAddress);
        Wire.write(txData);
        Wire.endTransmission();    // this is what actually sends the data

            // for demo show the data that as been sent
        Serial.print("Sent ");
        Serial.println(txData);
        // Serial.print(txData.textA);
        // Serial.print(' ');
        // Serial.print(txData.valA);
        // Serial.print(' ');
        // Serial.println(txData.valB);

        newTxData = false;
    }
}

//=============

void showNewData() {

    Serial.print("This just in    ");
    Serial.println(rxData);
    // Serial.print(rxData.textB);
    // Serial.print(' ');
    // Serial.print(rxData.valC);
    // Serial.print(' ');
    // Serial.println(rxData.valD);
}

//============

        // this function is called by the Wire library when a message is received
void receiveEvent(int numBytesReceived) {

    if (newRxData == false) {
            // copy the data to rxData
        //Wire.readBytes( (byte*) &rxData, numBytesReceived);
        rxData = Wire.read();
        newRxData = true;
    }
    else {
            // dump the data
        while(Wire.available() > 0) {
            byte c = Wire.read();
        }
    }
}