#include "sdio_sd.hpp"
#include <cstdio>

static constexpr const char *TAG = "Sdio_SD";
static constexpr const char *LOG_FILE_PATH = "0:/log.bin";

FIL Sdio_SD::file = {};
bool Sdio_SD::file_open = false;
bool Sdio_SD::mounted = false;

static bool write_checked(const void *ptr,
                          size_t size,
                          size_t count,
                          FIL *file,
                          const char *label)
{
    UINT bytes_written = 0;

    const UINT total_bytes = static_cast<UINT>(size * count);
    const FRESULT res = f_write(file, ptr, total_bytes, &bytes_written);

    if (res != FR_OK || bytes_written != total_bytes)
    {
        printf("[%s] WRITE FAILED [%s] expected=%u written=%u res=%d\r\n",
               TAG,
               label,
               static_cast<unsigned>(total_bytes),
               static_cast<unsigned>(bytes_written),
               static_cast<int>(res));

        return false;
    }

    return true;
}

FRESULT Sdio_SD::init()
{
    FRESULT ret;

    // Mount SD card
    ret = f_mount(&SDFatFS, SDPath, 1);
    if (ret != FR_OK)
    {
        printf("[%s] Mount failed: %d\r\n", TAG, static_cast<int>(ret));
        mounted = false;
        return ret;
    }

    mounted = true;
    printf("[%s] SD mounted\r\n", TAG);

    // Open log file
    ret = f_open(&file, LOG_FILE_PATH, FA_CREATE_ALWAYS | FA_WRITE);
    if (ret != FR_OK)
    {
        printf("[%s] File open failed: %d\r\n", TAG, static_cast<int>(ret));
        file_open = false;

        // keep behavior safe: if file open fails after mount, unmount
        f_mount(nullptr, "", 1);
        mounted = false;

        return ret;
    }

    file_open = true;
    printf("[%s] File opened\r\n", TAG);

    return FR_OK;
}

size_t Sdio_SD::write(const BUFFER_HANDLER::BUFFER &buf, uint32_t count)
{
    if (!file_open)
        return 0;

    LogHeader hdr;
    hdr.sample_count = count;

    bool ok = true;

    ok &= write_checked(&hdr, sizeof(hdr), 1, &file, "header");

    ok &= write_checked(buf.u.data(), sizeof(float), count, &file, "u");
    ok &= write_checked(buf.Va.data(), sizeof(float), count, &file, "Va");
    ok &= write_checked(buf.Vb.data(), sizeof(float), count, &file, "Vb");
    ok &= write_checked(buf.current.data(), sizeof(float), count, &file, "current");
    ok &= write_checked(buf.ticks.data(), sizeof(int32_t), count, &file, "ticks");

    if (ok)
        printf("[%s] WRITE %lu samples OK\r\n", TAG, static_cast<unsigned long>(count));
    else
        printf("[%s] WRITE FAILED\r\n", TAG);

    return ok;
}

size_t Sdio_SD::write_test_pattern()
{
    if (!file_open)
    {
        printf("[%s] File not open\r\n", TAG);
        return 0;
    }

    const uint8_t test_data[] = {1, 2, 3, 4, 5, 6};

    UINT bytes_written = 0;
    const FRESULT res = f_write(&file,
                                test_data,
                                sizeof(test_data),
                                &bytes_written);

    if (res != FR_OK || bytes_written != sizeof(test_data))
    {
        printf("[%s] TEST WRITE FAILED expected=%u written=%u res=%d\r\n",
               TAG,
               static_cast<unsigned>(sizeof(test_data)),
               static_cast<unsigned>(bytes_written),
               static_cast<int>(res));
    }
    else
    {
        printf("[%s] TEST WRITE OK (%u bytes)\r\n",
               TAG,
               static_cast<unsigned>(bytes_written));
    }

    return bytes_written;
}

void Sdio_SD::flush(uint8_t mode)
{
    if (!file_open)
    {
        printf("[%s] Flush called but file is not open\r\n", TAG);
        return;
    }

    // ===== FAST PATH =====
    if (mode == 0)
    {
        f_sync(&file);
        return;
    }

    // ===== DEBUG PATH =====
    printf("[%s] Final flush (debug mode)...\r\n", TAG);

    const FRESULT res = f_sync(&file);

    if (res != FR_OK)
    {
        printf("[%s] Flush FAILED res=%d\r\n", TAG, static_cast<int>(res));
    }
    else
    {
        printf("[%s] Flush OK\r\n", TAG);
    }
}

void Sdio_SD::close()
{
    if (!file_open)
    {
        printf("[%s] Close called but file already closed\r\n", TAG);
        return;
    }

    printf("[%s] Closing file...\r\n", TAG);

    const FRESULT res = f_close(&file);

    if (res != FR_OK)
    {
        printf("[%s] Close FAILED res=%d\r\n", TAG, static_cast<int>(res));
    }
    else
    {
        printf("[%s] File closed successfully\r\n", TAG);
    }

    file_open = false;
}

void Sdio_SD::unmount_safe()
{
    printf("[%s] Starting safe SD unmount...\r\n", TAG);

    // Close file if still open
    if (file_open)
    {
        printf("[%s] File still open — closing before unmount\r\n", TAG);
        close();
    }

    if (!mounted)
    {
        printf("[%s] Card already unmounted or not mounted\r\n", TAG);
        return;
    }

    const FRESULT ret = f_mount(nullptr, "", 1);

    if (ret != FR_OK)
    {
        printf("[%s] Unmount FAILED: %d\r\n", TAG, static_cast<int>(ret));
    }
    else
    {
        printf("[%s] SD card unmounted successfully\r\n", TAG);
    }

    mounted = false;
}