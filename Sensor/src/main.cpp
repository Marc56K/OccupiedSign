#include <Arduino.h>
#include "VL53L0X.h"
#include "LowPower.h"
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define LED_PIN 2
#define RF24_VCC_PIN A0
#define RF24_CE_PIN 9
#define RF24_CSN_PIN 10
#define TOF_VCC_PIN A1

RF24 radio(RF24_CE_PIN, RF24_CSN_PIN); // CE, CSN
uint32_t updateCounter = 0;

void setup()
{
    Serial.begin(9600);
    Serial.println("RF24_CHANNEL: " + String(RF24_CHANNEL));
    Serial.println("SENSOR_IDX: " + String(SENSOR_IDX));
    Serial.println("MAX_DISTANCE_MM: " + String(MAX_DISTANCE_MM));

    pinMode(LED_PIN, OUTPUT);
    pinMode(RF24_VCC_PIN, OUTPUT);
    pinMode(TOF_VCC_PIN, OUTPUT);

    Serial.println("started");
}

int32_t getSensorState()
{
    int32_t result = 0;
    digitalWrite(TOF_VCC_PIN, HIGH);
    {
        int32_t maxDist = 0;
        for (uint32_t i = 0; i < 3; i++)
        {
            VL53L0X sensor;
            Wire.begin();
            if (sensor.init())
            {
                sensor.setTimeout(500);
                sensor.writeReg(0x80, 0x01);
                int32_t dist = (int32_t)sensor.readRangeSingleMillimeters();
                dist = min(2000, dist);
                Serial.print(dist);
                Serial.println("mm");
                maxDist = max(maxDist, dist);
                if (dist >= MAX_DISTANCE_MM)
                {
                    break;
                }
            }
            else
            {
                Serial.println("Sensor init failed");
            }
        }
        
        if (maxDist > 0 && maxDist < MAX_DISTANCE_MM)
        {
            result = 1;
        }
    }
    digitalWrite(TOF_VCC_PIN, LOW);
    pinMode(A4, INPUT);
    pinMode(A5, INPUT);

    return result;
}

bool sendState(int32_t state)
{
    int success = 0;
    digitalWrite(RF24_VCC_PIN, HIGH);
    {
        uint64_t address = static_cast<uint64_t>(RF24_BASE_ADDRESS) + SENSOR_IDX;

        radio.begin();
        radio.setRetries(15, 15);
        radio.setDataRate(RF24_250KBPS);
        radio.setPALevel(RF24_PA_MAX);
        radio.setChannel(RF24_CHANNEL);
        radio.setPayloadSize(sizeof(int32_t));
        radio.openWritingPipe(address);
        radio.stopListening();

        Serial.print("sending");
        auto endTime = millis() + 20000;
        while(millis() < endTime && success < 2)
        {
            Serial.print(".");
            if (radio.write(&state, sizeof(state)))
            {
                success++;
            }
            delay(1);
        }
        Serial.println(success > 1 ? " success" : " timeout");
        radio.powerDown();
    }
    digitalWrite(RF24_VCC_PIN, LOW);
    return success > 1;
}

void loop()
{
    bool error = false;
    int32_t sensorState = -1;
    digitalWrite(LED_PIN, HIGH);
    {
        updateCounter++;
        updateCounter = min(updateCounter, HEARTBEAT_INTERVAL);

        sensorState = getSensorState();
        Serial.println(sensorState);
        if (sensorState != 0 || updateCounter >= HEARTBEAT_INTERVAL)
        {
            if (sendState(sensorState))
            {
                updateCounter = 0;
            }
            else
            {
                error = true;
            }
        }
    }
    if (error)
    {
        digitalWrite(LED_PIN, LOW);
        delay(100);
        digitalWrite(LED_PIN, HIGH);
        delay(100);
    }
    digitalWrite(LED_PIN, LOW);

    Serial.flush();

    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
}
