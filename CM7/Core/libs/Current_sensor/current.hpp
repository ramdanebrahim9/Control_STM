#pragma once
#include <stdint.h>

class CurrentSensor
{
public:
    CurrentSensor(volatile uint16_t *adc_buffer);

    float readVoltage() const;
    float readCurrent() const;

    void calibrate();

private:
    volatile uint16_t *adc_;

    float offset_v_ = 0.0f;

    static constexpr float VREF = 3.3f;
    static constexpr float ADC_MAX = 65535.0f;

    // For a 20A/V sensor with a 0.01Ω shunt resistor
    static constexpr float CURRENT_GAIN = 0.2f; // 0.01Ω * 20
};