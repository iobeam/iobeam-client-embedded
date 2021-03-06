/*
 * This sketch shows a basic example of how to use the iobeam library in an
 * Arduino project.
 *
 * The setup() initializes the Ethernet and the iobeam library,
 * registering the device if necessary.
 *
 * The loop() reads the analog value of pin 0 and transmits this value to 
 * the iobeam cloud every 15 seconds. For real projects, you would replace
 * this logic with code to measure whatever values you are concerned with.
 */
#define __STDC_LIMIT_MACROS

#include <SPI.h>
#include <EEPROM.h>
#include <YunClient.h>

#define API_DEFAULT_SERVER "api.iobeam.com"
#define DEBUG_LEVEL 0
#include <Iobeam.h>

// Necessary for iobeam. The project token should be PROGMEM to save RAM.
#define PROJECT_ID -1  // YOUR PROJECT ID
PROGMEM const char token[] = {"YOUR PROJECT TOKEN"};

// Initialize the network and iobeam libraries.
YunClient client;
Iobeam iobeam(client);

void setup() {
  setupSerial();
  setupNetwork();
  
  // iobeam initialization
  iobeam.init(PROJECT_ID, token, 0);
  iobeam.startTimeKeeping();
  if (iobeam.registerDevice(0) < 0) {
    errorForever();
  }
  
  IOBEAM_VERBOSE("\n");
  IOBEAM_LOG("Setup done.\n");
}

void loop() {
  int temp = analogRead(0);
  boolean success = iobeam.send("analog", temp);
  IOBEAM_VERBOSE("\n");
  
  IOBEAM_LOG("import success: ");
  IOBEAM_LOG(success);
  IOBEAM_LOG("\n");
  delay(15000);
}

void setupSerial() {
  if (DEBUG_LEVEL > 0) {
    Serial.begin(9600);
    while (!Serial); // wait for serial port to connect. Needed for Leonardo only
  }
}

void setupNetwork() {
  Bridge.begin();
  int tries = 0;
  while (1) {
    int ret = client.connect("iobeam.com", 80);
    client.stop();
    if (ret > 0)
      break;
    delay(1000);
    IOBEAM_ERR("Failed to connect, retry...\n");
    tries++;
    if (tries > 30)
      break;
  }
  delay(1000);  // give Client a moment to init  
}

void errorForever() {
  IOBEAM_ERR("Error, looping forever.\n");
  while(1);
}


