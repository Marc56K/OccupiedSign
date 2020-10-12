#pragma once
#include "SensorData.h"
#include <epd4in2.h>
#include "epdpaint.h"
#include <images.h>

class Display
{
public:
    Display(const uint8_t vccPin);
    ~Display();

    bool Update(const SensorData& sensorData);

private:
    const uint8_t _vccPin;
    Epd _epd;
    SensorData _currSensorData;
};