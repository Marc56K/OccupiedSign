#include <Arduino.h>
#include "Display.h"
#include "LowPower.h"
#include <nRF24L01.h>
#include <RF24.h>
#include <SPI.h>

#define LED_PIN 2
#define RF24_VCC_PIN A0
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
    pinMode(RF24_VCC_PIN, OUTPUT);

    for (uint8_t sensor = 0; sensor < NUM_SENSORS; sensor++)
        sensorOfflineCounter[sensor] = 0;

    Serial.println("started");
}

bool getSensorData(SensorData& sensorData)
{
    bool receivedData = false;
    digitalWrite(RF24_VCC_PIN, HIGH);
    {  
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

        if (minOfflineCount > 2)
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
            while (radio.available(&pipe))
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
    }
    digitalWrite(RF24_VCC_PIN, LOW);

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