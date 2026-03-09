#include "current.hpp"
#include <cstdio>

extern "C"
{
#include "main.h"

    extern ADC_HandleTypeDef hadc2;
    extern TIM_HandleTypeDef htim1;
}

volatile uint16_t adc_buffer[1];

HAL_StatusTypeDef CurrentSensor::start()
{
    HAL_StatusTypeDef status;

    printf("CurrentSensor init...\r\n");

    status = HAL_ADCEx_Calibration_Start(&hadc2, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);
    if (status != HAL_OK)
    {
        printf("ADC calibration failed\r\n");
        return status;
    }

    status = HAL_ADC_Start_DMA(&hadc2, (uint32_t *)adc_buffer, 1);
    if (status != HAL_OK)
    {
        printf("ADC DMA start failed\r\n");
        return status;
    }

    status = HAL_TIM_Base_Start_IT(&htim1);
    if (status != HAL_OK)
    {
        printf("Timer start failed\r\n");
        return status;
    }

    calibrate();

    return HAL_OK;
}

CurrentSensor::CurrentSensor() {}

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