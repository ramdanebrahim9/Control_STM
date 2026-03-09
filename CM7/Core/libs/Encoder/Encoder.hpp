#pragma once

#include "stm32h7xx_hal.h"
#include <stdint.h>

extern TIM_HandleTypeDef htim2;

class Encoder
{
public:
    Encoder();

    HAL_StatusTypeDef start();
    HAL_StatusTypeDef stop();
    void clear();

    int32_t getCount() const;

private:
    inline static TIM_HandleTypeDef *timer_ = &htim2;
};