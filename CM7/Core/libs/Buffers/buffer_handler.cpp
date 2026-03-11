// ============================================================
//  * File: buffer_handler.cpp
// ============================================================

#include "buffer_handler.hpp"
#include <cstring>
#include <cstdio>

static const char *TAG = "BUFFER";

/* ===================== SD_Transfer Task ===================== */

// static void task_callback(void *arg)
// {
//     (void)arg;

//     while (true)
//     {
//         uint32_t notify_value = 0;

//         xTaskNotifyWait(
//             0,
//             UINT32_MAX,
//             &notify_value,
//             portMAX_DELAY);

//         if (notify_value & static_cast<uint32_t>(BUFFER_HANDLER::Send_Type::FULL))
//         {
//             auto &buf = BUFFER_HANDLER::getReadBuffer();
//             // Sdio_SD::write(buf, BUFFER_HANDLER::samples);
//             // Sdio_SD::flush();
//         }

//         if (notify_value & static_cast<uint32_t>(BUFFER_HANDLER::Send_Type::LAST))
//         {
//             auto &buf = BUFFER_HANDLER::getWriteBuffer();

//             int remaining = BUFFER_HANDLER::write_index;

//             printf("%s: inside Last : remaining is %d\r\n", TAG, remaining);

//             if (remaining > 0)
//             {
//                 // Sdio_SD::write(buf, remaining);
//                 //  Sdio_SD::flush(1);

//                 printf("%s: Final partial write: %d samples\r\n", TAG, remaining);
//             }

//             break; // exit AFTER handling LAST
//         }
//     }

//     printf("%s: SD task exiting\r\n", TAG);
//     vTaskDelete(nullptr);
// }

void BUFFER_HANDLER::init()
{
    // xTaskCreate(
    //     task_callback, // task entry function
    //     "SD_task",     // name (for debugging)
    //     4096,          // stack size (words)
    //     nullptr,       // argument pointer
    //     3,             // priority
    //     &task_handle   // handle storage
    // );

    // Sdio_SD::init();
}

/* ===================== STATIC STATE ===================== */

volatile BUFFER_HANDLER::Target_Buffer BUFFER_HANDLER::current_buffer = BUFFER_HANDLER::Target_Buffer::BUFFER_1;

int BUFFER_HANDLER::write_index = 0;

BUFFER_HANDLER::BUFFER BUFFER_HANDLER::Buffer1{};
BUFFER_HANDLER::BUFFER BUFFER_HANDLER::Buffer2{};

/* ===================== INTERNAL HELPERS ===================== */

BUFFER_HANDLER::BUFFER &BUFFER_HANDLER::getWriteBuffer()
{
    // ESP_LOGD(TAG, "Access write buffer");
    return (current_buffer == Target_Buffer::BUFFER_1) ? Buffer1 : Buffer2;
}

BUFFER_HANDLER::BUFFER &BUFFER_HANDLER::getReadBuffer()
{
    // ESP_LOGD(TAG, "Access read buffer");
    return (current_buffer == Target_Buffer::BUFFER_1) ? Buffer2 : Buffer1;
}

void BUFFER_HANDLER::switchBuffer()
{
    current_buffer = (current_buffer == Target_Buffer::BUFFER_1) ? Target_Buffer::BUFFER_2 : Target_Buffer::BUFFER_1;

    write_index = 0;

    // const uint8_t buf_num = (current_buffer == Target_Buffer::BUFFER_1) ? 1 : 2;

    // ESP_LOGI(TAG, "Switched to Buffer %u", (unsigned)buf_num);
}

void BUFFER_HANDLER::clearBuffer(BUFFER &buf)
{
    // Clear all channels
    memset(buf.u.data(), 0, sizeof(float) * samples);
    memset(buf.Va.data(), 0, sizeof(float) * samples);
    memset(buf.Vb.data(), 0, sizeof(float) * samples);
    memset(buf.current.data(), 0, sizeof(float) * samples);
    memset(buf.ticks.data(), 0, sizeof(int32_t) * samples);

    // LOGI Succes clearence
    const uint8_t buf_num = (current_buffer == Target_Buffer::BUFFER_1) ? 1 : 2;
    printf("%s: Buffer cleared %u\r\n", TAG, (unsigned)buf_num);
}

void BUFFER_HANDLER::FillBuffer(const Sample_Data &Data)
{
    BUFFER_HANDLER::filled += 1;

    BUFFER &buf = getWriteBuffer();

    buf.u[write_index] = Data.u;
    buf.Va[write_index] = Data.Va;
    buf.Vb[write_index] = Data.Vb;
    buf.current[write_index] = Data.current;
    buf.ticks[write_index] = Data.ticks;

    write_index++;

    if (write_index >= samples)
    {
        BUFFER_HANDLER::Write_flag += 1;

        BUFFER_HANDLER::switchBuffer();

        // wake up SD_transfer
        // xTaskNotify(
        //     BUFFER_HANDLER::task_handle,
        //     static_cast<uint32_t>(BUFFER_HANDLER::Send_Type::FULL),
        //     eSetValueWithOverwrite);
    }
}

void BUFFER_HANDLER::flushRemaining()
{
    // xTaskNotify(
    //     BUFFER_HANDLER::task_handle,
    //     static_cast<uint32_t>(BUFFER_HANDLER::Send_Type::LAST),
    //     eSetValueWithOverwrite);
}