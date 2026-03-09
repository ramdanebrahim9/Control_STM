extern "C"
{
#include "main.h"
}

#include "app.hpp"
#include "Encoder.hpp"
#include <stdio.h>

Encoder enc1;

extern "C" void app_main(void)
{
    printf("APP : ---------- STM H7! ----------\n\r");

    if (enc1.start() != HAL_OK)
    {
        printf("Encoder start failed\n");

        while (1)
        {
            // error state
        }
    }
    printf("Encoder started successfully\n");

    while (1)
    {
        int32_t pos = enc1.getCount();

        printf("pos: %ld\n", pos);

        HAL_Delay(50);
    }
}
