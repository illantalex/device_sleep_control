#include <Arduino.h>
#include <Wire.h>
#include <EEPROM.h>

#define SDA_PIN 14
#define SCL_PIN 12
#define POWER_CTL_PIN 4
#define RUN_PIN 2

#define EEPROM_SIZE 8
#define EEPROM_ADDR 0

// #define DEBUG

uint64_t deepSleepTime = 30LL * 60LL * 1000000LL;
uint64_t deepSleepTimeBytes = 0;
bool receivedSleep = false;

void requestCallback() {
    // Add your code to handle the request here
    // You can send data back to the master using the Wire library
    Wire.print("OK");
}

void receiveCallback(int byteCount) {
    // Add your code to handle the received data here
    // You can read the received data using the Wire library

    while (Wire.available()) {
        // Write 8 bytes from I2C as new deepSleepTime
        int command = Wire.read();
        switch (command) {
        case 0x01: {
            receivedSleep = true;
            break;
        }
        case 0x02: {
                if (byteCount == 9) {
                    // Check if the new deepSleepTime differs from the old one
                    deepSleepTimeBytes = 0;
                    for (int i = 7; i >= 0; i--) {
                        deepSleepTimeBytes |= ((uint64_t)Wire.read() << (i * 8));
                    }
#ifdef DEBUG
                    Serial.println(deepSleepTimeBytes, HEX);
#endif
                    // Write new deepSleepTime to EEPROM if it differs from the old one
                    if (deepSleepTimeBytes != deepSleepTime) {
                        EEPROM.put(EEPROM_ADDR, deepSleepTimeBytes);
                        EEPROM.commit();
                        EEPROM.get(EEPROM_ADDR, deepSleepTime);
                    }
                }
                break;
            }
        default:
            break;
        }
    }
}

void setup() {
    // Add your setup code here
    pinMode(RUN_PIN, OUTPUT);
    digitalWrite(RUN_PIN, LOW);
    pinMode(POWER_CTL_PIN, OUTPUT);
    digitalWrite(POWER_CTL_PIN, HIGH);
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.get(EEPROM_ADDR, deepSleepTime);

#ifdef DEBUG
    Serial.begin(115200);
    Serial.println("I2C Slave Example");
    Serial.println(deepSleepTime, HEX);
#endif

    Wire.begin(SDA_PIN, SCL_PIN, 0x42);
    Wire.setClock(25000L);
    Wire.onRequest(requestCallback);
    Wire.onReceive(receiveCallback);

    // Go to sleep if receivedSleep is true or when 10 minutes have passed
    while (!receivedSleep && millis() < 600){
        delay(100);
    }
    delay(20000); // wait for the raspberry pi to be totally power off
#ifdef DEBUG
    Serial.println("Going to sleep");
    Serial.println(deepSleepTime, HEX);
#endif
    digitalWrite(POWER_CTL_PIN, LOW);
    // sleep for 30 minutes or deepSleepTime
    if (deepSleepTime > 0 && deepSleepTime < ESP.deepSleepMax()) {
        ESP.deepSleep(deepSleepTime, RF_DISABLED);
    }
    ESP.deepSleep(ESP.deepSleepMax(), RF_DISABLED);
}

void loop() {
    // Add your loop code here
    delay(1000);
}
