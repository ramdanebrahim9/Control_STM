extern "C"
{
#include "main.h"
}

#include "app.hpp"
#include "Encoder.hpp"
#include "current.hpp"
#include "pwm_engine.hpp"
#include "buffer_handler.hpp"
#include "control.hpp"
#include "Macros.hpp"
#include <stdio.h>

#define INF_LOOP        \
    while (1)           \
    {                   \
        HAL_Delay(500); \
    }

Encoder enc1;
CurrentSensor current;
constexpr uint32_t dl = 500; // ms

extern "C" void app_main(void)
{
    // printf("APP : ---------- STM H7! ----------\r\n");
    printf(
        "Timer config | T_desired=%.2f s | Ts=%.6f s | TOTAL_SAMPLES=%lu | TOGGLE_SAMPLES=%lu\r\n",
        T_desired,
        Ts,
        TOTAL_SAMPLES,
        TOGGLE_SAMPLES);

    if (HAL_TIM_Base_Start_IT(&htim1) != HAL_OK)
    {
        printf("Timer start failed\n");
        INF_LOOP;
    }
    printf("MAIN : Timer1 started successfully\n");

    if (PWMEngine::start() != HAL_OK)
    {
        printf("PWM start failed\n");
        INF_LOOP;
    }
    printf("PWM started successfully\n");

    if (enc1.start() != HAL_OK)
    {
        printf("Encoder start failed\n");
        INF_LOOP;
    }
    printf("Encoder started successfully\n");

    if (current.start() != HAL_OK)
    {
        printf("Current sensor start failed\n");
        INF_LOOP;
    }
    printf("Current sensor ready\n");

    while (1)
    {
        float v = current.readVoltage();
        float i = current.readCurrent();

        printf("V = %.3f V   I = %.3f A\r\n", v, i);
        // ----------------------------------- //

        int32_t pos = enc1.getCount();

        printf("pos: %ld\n", pos);

        // HAL_Delay(50);

        PWMEngine::set_duty(2.0f); // command 2.0V
        HAL_Delay(dl);

        PWMEngine::set_duty(-4.0f); // command -4.0V
        HAL_Delay(dl);

        PWMEngine::set_duty(6.0f); // command 6.0V
        HAL_Delay(dl);

        PWMEngine::set_duty(-8.0f); // command -8.0V
        HAL_Delay(dl);

        PWMEngine::set_duty(11.0f); // command 11.0V
        HAL_Delay(dl);
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{

    if (htim->Instance != TIM1)
        return;

    volatile static uint32_t sample_counter = 0;
    sample_counter++;

    control_loop_step();

#ifdef STOP_EXP_OnTime
    if (sample_counter >= TOTAL_SAMPLES)
    {
        HAL_TIM_Base_Stop_IT(&htim1);
        HAL_ADC_Stop_DMA(&hadc2);
        BSP_LED_Toggle(LED_RED);
    };
#endif
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance != ADC2)
        return;

    static uint32_t counter = 0;
    counter++;
    if (counter >= TOGGLE_SAMPLES)
    {
        BSP_LED_Toggle(LED_RED);
        counter = 0;
    }
}