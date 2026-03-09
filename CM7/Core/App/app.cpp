extern "C"
{
#include "main.h"
}

#include "app.hpp"
#include "Encoder.hpp"
#include "current.hpp"
#include <stdio.h>

#define INF_LOOP        \
    while (1)           \
    {                   \
        HAL_Delay(500); \
    }

Encoder enc1;
CurrentSensor current;

extern "C" void app_main(void)
{
    printf("APP : ---------- STM H7! ----------\r\n");

    if (current.start() != HAL_OK)
    {
        printf("Current sensor start failed\n");
        INF_LOOP;
    }
    printf("Current sensor started successfully\n");

    if (enc1.start() != HAL_OK)
    {
        printf("Encoder start failed\n");
        INF_LOOP;
    }
    printf("Encoder started successfully\n");

    while (1)
    {
        float v = current.readVoltage();
        float i = current.readCurrent();

        printf("V = %.3f V   I = %.3f A\r\n", v, i);
        // ----------------------------------- //

        int32_t pos = enc1.getCount();

        printf("pos: %ld\n", pos);

        HAL_Delay(50);
    }
}