#pragma once

extern "C"
{
#include "main.h"
}

class PWMEngine
{
public:
    static HAL_StatusTypeDef start();
    static void kill();

    // voltage command → PWM duty
    static void set_duty(float v, float v_max = 12.0f);

private:
    static constexpr uint32_t PWM_PERIOD = 50 - 1; // 50us period → 20kHz PWM frequency
};