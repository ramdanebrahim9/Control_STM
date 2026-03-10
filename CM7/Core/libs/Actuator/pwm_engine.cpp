#include "pwm_engine.hpp"

extern TIM_HandleTypeDef htim1;

HAL_StatusTypeDef PWMEngine::start()
{
    HAL_StatusTypeDef status = HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4); // start PWM output
    if (status != HAL_OK)
    {
        return status;
    }
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, 0); // initialize duty to 0
    return HAL_OK;
}

void PWMEngine::kill()
{
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, 0); // set duty to 0
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_4);         // stop PWM output
}

void PWMEngine::set_duty(float v, float v_max)
{
    ;
    HAL_GPIO_WritePin(PWM_Direc_GPIO_Port, PWM_Direc_Pin, (v >= 0.0f) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // clamp voltage
    if (v < 0.0f)
        v = -v;

    float duty = v / v_max;

    uint32_t ticks = static_cast<uint32_t>(duty * PWM_PERIOD);

    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, ticks);
}