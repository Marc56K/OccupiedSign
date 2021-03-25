/**
 * The MIT License (MIT)
 * 
 * Copyright (c) 2021 Marc Roßbach
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <Arduino.h>
#include "Display.h"
#include "LowPower.h"
#include <nRF24L01.h>
#include <RF24.h>
#include <SPI.h>

#define LED_PIN 2
#define RF24_CE_PIN 9
#define RF24_CSN_PIN 10
#define DISP_VCC_PIN A1

Display disp(DISP_VCC_PIN);
RF24 radio(RF24_CE_PIN, RF24_CSN_PIN); // CE, CSN
uint32_t sensorOfflineCounter[NUM_SENSORS];

void setup()
{
    Serial.begin(9600);

    pinMode(LED_PIN, OUTPUT);

    for (uint8_t sensor = 0; sensor < NUM_SENSORS; sensor++)
        sensorOfflineCounter[sensor] = 0;

    Serial.println("started");
}

bool getSensorData(SensorData &sensorData)
{
    bool receivedData = false;

    radio.begin();
    radio.setRetries(15, 15);
    radio.setDataRate(RF24_250KBPS);
    radio.setPALevel(RF24_PA_MAX);
    radio.setChannel(RF24_CHANNEL);
    radio.setPayloadSize(sizeof(int32_t));

    uint32_t minOfflineCount = UINT32_MAX;
    for (uint8_t sensor = 0; sensor < NUM_SENSORS; sensor++)
    {
        uint64_t address = static_cast<uint64_t>(RF24_BASE_ADDRESS) + sensor;
        uint8_t pipe = sensor + 1;
        radio.openReadingPipe(pipe, address);
        sensorOfflineCounter[sensor] = min(sensorOfflineCounter[sensor] + 1, 2 * HEARTBEAT_INTERVAL);
        minOfflineCount = min(minOfflineCount, sensorOfflineCounter[sensor]);
    }
    radio.startListening();
    delay(10);

    if (minOfflineCount > 3)
    {
        sensorData.State = SensorState::FREE;
    }

    uint32_t retries = 1;
    if (sensorData.State == SensorState::BUSY)
    {
        retries = 5;
    }

    for (uint32_t retry = 0; !receivedData && retry < retries; retry++)
    {
        if (retry > 0)
        {
            Serial.print(".");
            delay(400);
        }

        uint8_t pipe = 0;
        for (uint32_t i = 0; radio.available(&pipe) && i < NUM_SENSORS; i++)
        {
            uint8_t sensorIdx = pipe - 1;
            sensorOfflineCounter[sensorIdx] = 0;

            int32_t sensorState = 0;
            radio.read(&sensorState, sizeof(sensorState));

            if (sensorState == 0)
            {
                Serial.println(String("Sensor") + String((int)sensorIdx) + String(": Heartbeat"));
            }
            else if (sensorState == 1)
            {
                Serial.println(String("Sensor") + String((int)sensorIdx) + String(": Active"));
                sensorData.State = SensorState::BUSY;
                receivedData = true;
            }
        }
    }

    if (!receivedData)
    {
        Serial.println("no data");
    }

    radio.powerDown();
    digitalWrite(RF24_CE_PIN, HIGH);

    sensorData.Message = "";
    for (uint8_t sensor = 0; sensor < NUM_SENSORS; sensor++)
    {
        if (sensorOfflineCounter[sensor] > 1.5 * HEARTBEAT_INTERVAL)
        {
            sensorData.Message += String("Sensor_") + String((int)sensor) + String(" OFFLINE  ");
        }
    }

    return receivedData;
}

SensorData lastSensorData;
void loop()
{
    digitalWrite(LED_PIN, HIGH);
    {
        getSensorData(lastSensorData);
        disp.Update(lastSensorData);
    }
    digitalWrite(LED_PIN, LOW);

    Serial.flush();

    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
}