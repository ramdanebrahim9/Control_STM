#include "Encoder.hpp"

Encoder::Encoder() {}

HAL_StatusTypeDef Encoder::start()
{
    HAL_StatusTypeDef status;
    status = HAL_TIM_Encoder_Start(timer_, TIM_CHANNEL_ALL);
    return status;
}

HAL_StatusTypeDef Encoder::stop()
{
    HAL_StatusTypeDef status;
    status = HAL_TIM_Encoder_Stop(timer_, TIM_CHANNEL_ALL);
    return status;
}

void Encoder::clear()
{
    __HAL_TIM_SET_COUNTER(timer_, 0);
}

int32_t Encoder::getCount() const
{
    return (int32_t)__HAL_TIM_GET_COUNTER(timer_);
}