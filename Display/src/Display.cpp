#include "Display.h"

#ifdef WOMEN
    #define IMG_FREE IMG_women_free
    #define IMG_BUSY IMG_women_busy
#else
    #define IMG_FREE IMG_men_free
    #define IMG_BUSY IMG_men_busy
#endif

Display::Display(const uint8_t vccPin)
    : _vccPin(vccPin)
{
    pinMode(_vccPin, OUTPUT);
}

Display::~Display()
{
}

bool Display::Update(const SensorData& sensorData)
{
    SensorState newState = sensorData.State != SensorState::UNKNOWN ? sensorData.State : _currSensorData.State;
    if (_currSensorData.State == newState && _currSensorData.Message == sensorData.Message)
    {
        return false;
    }

    _currSensorData.State = newState;
    _currSensorData.Message = sensorData.Message;    

    digitalWrite(_vccPin, HIGH);
    {
        if (_epd.Init() == 0)
        {
            _epd.ClearFrame(true);

            switch (_currSensorData.State)
            {
                case SensorState::FREE:
                    _epd.SetPartialCompressedWindow(IMG_FREE, 0, 0, 400, 300);
                    break;
                case SensorState::BUSY:
                    _epd.SetPartialCompressedWindow(IMG_BUSY, 0, 0, 400, 300);
                    break;
                default:
                    break;
            }

            if (_currSensorData.Message.length() > 0)
            {
                unsigned char image[384 * 16 / 8];
                Paint paint(image, 380, 16);
                paint.Clear(0);
                paint.DrawStringAt(8, 6, _currSensorData.Message.c_str(), &Font8, 1);
                _epd.SetPartialWindow(paint.GetImage(), 10, 300 - 22, paint.GetWidth(), paint.GetHeight());
            }

            _epd.DisplayFrame(true);

            _epd.Sleep();   
        }
        else
        {
            Serial.println("e-Paper init failed");
        }
        digitalWrite(RST_PIN, LOW); // saves 3mA in deep sleep
    }
    digitalWrite(_vccPin, LOW);

    return true;
}