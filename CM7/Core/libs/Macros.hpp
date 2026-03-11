#pragma once

#include <cmath>
#include <cstdint>

// #define STOP_EXP_OnTime // Comment this line to STOP the experiment on TIMER done

extern ADC_HandleTypeDef hadc2;
extern TIM_HandleTypeDef htim1;

constexpr uint32_t TIMER_INTERVAL_US = 50;

constexpr double Ts = TIMER_INTERVAL_US * 1e-6;
constexpr double T_desired = 6.0;

constexpr uint32_t TOTAL_SAMPLES = static_cast<uint32_t>(T_desired / Ts);

constexpr uint32_t TOGGLE_SAMPLES = static_cast<uint32_t>(0.5 / Ts);
