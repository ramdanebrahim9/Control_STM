#pragma once
#include <stdint.h>

extern "C"
{
#include "main.h"
}

extern volatile uint16_t adc_buffer[1];

class CurrentSensor
{
public:
    CurrentSensor();

    float readVoltage() const;
    float readCurrent() const;

    HAL_StatusTypeDef start();
    void calibrate();

private:
    static inline volatile uint16_t *const adc_ = adc_buffer;

    float offset_v_ = 0.0f;

    static constexpr float VREF = 3.3f;
    static constexpr float ADC_MAX = 65535.0f;

    static constexpr float CURRENT_GAIN = 0.2f;
};