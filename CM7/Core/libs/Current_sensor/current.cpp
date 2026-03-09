#include "current.hpp"
#include <cstdio>

extern "C"
{
#include "main.h"
}

CurrentSensor::CurrentSensor(volatile uint16_t *adc_buffer)
    : adc_(adc_buffer)
{
}

float CurrentSensor::readVoltage() const
{
    return (*adc_) * (VREF / ADC_MAX);
}

float CurrentSensor::readCurrent() const
{
    float v = readVoltage();
    return (v - offset_v_) / CURRENT_GAIN;
}

void CurrentSensor::calibrate()
{
    constexpr int samples = 2000;
    float sum = 0.0f;

    printf("CurrentSensor: calibrating offset...\r\n");

    for (int i = 0; i < samples; i++)
    {
        sum += readVoltage();
        HAL_Delay(1);
    }

    offset_v_ = sum / samples;

    printf("Offset voltage = %.6f mV\r\n", offset_v_ * 1000.0f);
}