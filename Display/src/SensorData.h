#pragma once
#include <Arduino.h>

enum SensorState
{
    UNKNOWN = 0,
    FREE,
    BUSY
};

struct SensorData
{
    SensorState State;
    String Message;

    SensorData()
        : State(SensorState::UNKNOWN)
    {
    }
};
