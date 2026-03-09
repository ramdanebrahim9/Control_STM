extern "C"
{
#include "main.h"
}

#include "app.hpp"
#include "Encoder.hpp"
#include "current.hpp"
#include <stdio.h>

Encoder enc1;
extern volatile uint16_t adc_buffer[1];
CurrentSensor current(adc_buffer);

extern "C" void app_main(void)
{
    printf("APP : ---------- STM H7! ----------\r\n");

    current.calibrate(); // measure zero current offset
    HAL_Delay(10);       // wait a bit before starting measurements

    while (1)
    {
        float v = current.readVoltage();
        float i = current.readCurrent();

        printf("V = %.3f V   I = %.3f A\r\n", v, i);

        HAL_Delay(100);
    }
}

// extern "C" void app_main(void)
// {
//     printf("APP : ---------- STM H7! ----------\n\r");

//     if (enc1.start() != HAL_OK)
//     {
//         printf("Encoder start failed\n");

//         while (1)
//         {
//             // error state
//         }
//     }
//     printf("Encoder started successfully\n");

//     while (1)
//     {
//         int32_t pos = enc1.getCount();

//         printf("pos: %ld\n", pos);

//         HAL_Delay(50);
//     }
// }