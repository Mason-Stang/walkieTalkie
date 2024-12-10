
void runTests() {
  Serial.println("Running tests...");

  if (!testHandleNewData()) {
    Serial.println("Failed testHandleNewData");
    return;
  }
  if (!testButtonPushed()) {
    Serial.println("Failed testButtonPushed");
    return;
  }
  if (!testRequestData()) {
    Serial.println("Failed testRequestData");
    return;
  }
  if (!testSendFile()) {
    Serial.println("Failed testSendFile");
    return;
  }

  Serial.println("All tests passed");
}

bool testHandleNewData() {
  // First branch:
  newRxData = true;
  receivingFile = false;
  rxData.hasData = true;
  rxData.wait = false;
  handleNewData();
  if (!receivingFile || newRxData) {
    Serial.println("Failed branch one: ");
    return false;
  }

  // Second branch:
  newRxData = true;
  receivingFile = true;
  rxData.hasData = true;
  rxData.wait = false;
  handleNewData();
  if (!receivingFile || newRxData) {
    Serial.println("Failed branch two: ");
    return false;
  }

  // Third branch:
  newRxData = true;
  receivingFile = true;
  rxData.hasData = false;
  rxData.wait = false;
  handleNewData();
  if (receivingFile || newRxData) {
    Serial.println("Failed branch three: ");
    return false;
  }

  // Wait branch:
  newRxData = true;
  receivingFile = false;
  rxData.hasData = false;
  rxData.wait = true;
  handleNewData();
  if (receivingFile || newRxData) {
    Serial.println("Failed wait branch: ");
    return false;
  }

  return true;
}

bool testButtonPushed() {
  // First branch:
  recording_now = false;
  sendingFile = true;
  button_pushed();
  if (recording_now) {
    Serial.println("Failed branch one: ");
    return false;
  }

  // Second branch:
  sendingFile = false;
  rqSent = true;
  receivingFile = false;
  time_last_pushed = millis();
  recording_now = false;
  button_pushed();
  if (recording_now) {
    Serial.println("Failed branch two: ");
    return false;
  }

  // Third branch:
  time_last_pushed = millis()-1000;
  recording_now = false;
  button_pushed();
  if (!recording_now) {
    Serial.println("Failed branch three: ");
    return false;
  }

  // Fourth branch
  file_ready = false;
  time_last_pushed = millis()-1000;
  recording_now = true;
  button_pushed();
  if (recording_now || !file_ready) {
    Serial.println("Failed branch four: ");
    return false;
  }

  return true;
}

bool testRequestData() {
  // Test that first branch is hit
  newRxData = false;
  requestData();
  if (newRxData) {
    Serial.println("Failed to hit first branch: ");
    return;
  }

  return true;
}

bool testSendFile() {
  // Returns from function in trivial case
  sendingFile = false;
  sendFile();

  // Returns when numBytesToWrite = 0
  numBytesToWrite = 0;
  sendFile();

  // Returns when numBytesToWrite < MAX_BUF_SIZE
  numBytesToWrite = MAX_BUF_SIZE - 10;
  sendFile();

  // Returns when numBytesToWrite = MAX_BUF_SIZE
  numBytesToWrite = MAX_BUF_SIZE;
  sendFile();

  // Returns when numBytesToWrite > MAX_BUF_SIZE
  numBytesToWrite = MAX_BUF_SIZE * 5 + 10;
  sendFile();

  return true;
}
