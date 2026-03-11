// ============================================================
//  * File: buffer_handler.hpp
// ============================================================

#pragma once

#include <array>
#include <cstdint>

extern "C"
{
#include "main.h"
}

struct Sample_Data
{
    float u;
    float Va;
    float Vb;
    float current;
    int32_t ticks;
};

// Ping-pong buffer for non-blocking data logging (static only)
class BUFFER_HANDLER
{
public:
    /* ===================== TYPES ===================== */

    enum class Target_Buffer : uint8_t
    {
        BUFFER_1,
        BUFFER_2
    };

    enum class Send_Type : uint32_t
    {
        FULL = 1 << 0, // 00000001 → value = 1
        LAST = 1 << 1  // 00000010 → value = 2
    };

    /* ===================== CONFIG ===================== */

    static constexpr int samples = 1000;

    static inline uint32_t filled = 0;

    static inline uint32_t Write_flag = 0;

    /* ===================== BUFFER ===================== */

    struct BUFFER
    {
        std::array<float, samples> u{};
        std::array<float, samples> Va{};
        std::array<float, samples> Vb{};
        std::array<float, samples> current{};
        std::array<int32_t, samples> ticks{};
    };

    /* ===================== GLOBAL STATE ===================== */

    static volatile Target_Buffer current_buffer;
    static int write_index;

    static BUFFER Buffer1;
    static BUFFER Buffer2;

    /* ===================== API ===================== */

    static BUFFER &getWriteBuffer();
    static BUFFER &getReadBuffer();
    static void switchBuffer();
    static void clearBuffer(BUFFER &buf);
    static void FillBuffer(const Sample_Data &Data);
    static void flushRemaining();

    /* ===================== SD_Transfer Task ===================== */
    static void init();
};