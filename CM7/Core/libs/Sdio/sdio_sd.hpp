#pragma once

#include <cstddef>
#include <cstdint>
#include "buffer_handler.hpp"

extern "C"
{
#include "fatfs.h"
#include "ff.h"
}

struct LogHeader
{
    uint32_t magic = 0xABCD1234;
    uint32_t version = 1;
    uint32_t sample_count;
};

class Sdio_SD
{
public:
    // Initialize + mount SD card
    static FRESULT init();

    // Write raw buffer to file
    static size_t write(const BUFFER_HANDLER::BUFFER &buf, uint32_t count);
    // static size_t write(const BUFFER_HANDLER::BUFFER &buf);

    // Flush file buffers
    static void flush(uint8_t mode = 0);

    // Close file
    static void close();

    // Write test
    static size_t write_test_pattern();

    // Unmount_safe SD Cardd
    static void unmount_safe();

private:
    static FIL file;
    static bool file_open;
    static bool mounted;
};