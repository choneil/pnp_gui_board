/**
  ******************************************************************************
  * @file    app_filex.c
  * @brief   FileX storage service on the microSD card (SDMMC2), ThreadX-based.
  ******************************************************************************
  */
#include "app_filex.h"
#include "fx_stm32_sd_driver.h"
#include "main.h"

#define FX_APP_THREAD_STACK_SIZE   (2 * 1024)
#define FX_APP_THREAD_PRIO         10
#define MOUNT_RETRY_TICKS          TX_TIMER_TICKS_PER_SECOND

/* Request flags handed to the storage thread. */
#define EVT_RESCAN      (1u << 0)
#define EVT_USB_ATTACH  (1u << 1)
#define EVT_USB_DETACH  (1u << 2)

static TX_THREAD fx_app_thread;
static TX_EVENT_FLAGS_GROUP storage_events;

/* Media buffer: one SD sector, 32-byte aligned for IDMA + cache maintenance. */
ALIGN_32BYTES(static uint32_t fx_sd_media_memory[FX_STM32_SD_DEFAULT_SECTOR_SIZE / sizeof(uint32_t)]);
static FX_MEDIA sdio_disk;

/* Root-directory snapshot (single writer: storage thread; readers copy). */
static volatile storage_state_t storage_state = STORAGE_NO_CARD;
static volatile UINT file_count = 0;
static char  file_names[STORAGE_MAX_FILES][STORAGE_MAX_NAME_LEN];
static ULONG file_sizes[STORAGE_MAX_FILES];

static VOID storage_thread_entry(ULONG input);
static VOID scan_root_directory(VOID);

UINT MX_FileX_Init(VOID *memory_ptr)
{
    TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL*)memory_ptr;
    VOID *stack;

    if (tx_event_flags_create(&storage_events, "storage events") != TX_SUCCESS)
    {
        return TX_GROUP_ERROR;
    }
    if (tx_byte_allocate(byte_pool, &stack, FX_APP_THREAD_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
    {
        return TX_POOL_ERROR;
    }
    if (tx_thread_create(&fx_app_thread, "FileX storage", storage_thread_entry, 0,
                         stack, FX_APP_THREAD_STACK_SIZE,
                         FX_APP_THREAD_PRIO, FX_APP_THREAD_PRIO,
                         TX_NO_TIME_SLICE, TX_AUTO_START) != TX_SUCCESS)
    {
        return TX_THREAD_ERROR;
    }

    /* Initialize FileX system once. */
    fx_system_initialize();
    return TX_SUCCESS;
}

storage_state_t storage_get_state(void) { return storage_state; }
UINT storage_get_file_count(void)       { return (storage_state == STORAGE_MOUNTED) ? file_count : 0; }

const char* storage_get_file_name(UINT i)
{
    return (i < file_count) ? file_names[i] : "";
}

ULONG storage_get_file_size(UINT i)
{
    return (i < file_count) ? file_sizes[i] : 0;
}

VOID storage_request_rescan(void) { tx_event_flags_set(&storage_events, EVT_RESCAN, TX_OR); }
VOID storage_usb_attach(void)     { tx_event_flags_set(&storage_events, EVT_USB_ATTACH, TX_OR); }
VOID storage_usb_detach(void)     { tx_event_flags_set(&storage_events, EVT_USB_DETACH, TX_OR); }

static VOID scan_root_directory(VOID)
{
    CHAR  name[FX_MAX_LONG_NAME_LEN];
    UINT  attributes;
    ULONG size;
    UINT  n = 0;
    UINT  status;

    status = fx_directory_first_full_entry_find(&sdio_disk, name, &attributes,
                                                &size, NULL, NULL, NULL, NULL, NULL, NULL);
    while ((status == FX_SUCCESS) && (n < STORAGE_MAX_FILES))
    {
        if ((attributes & (FX_DIRECTORY | FX_VOLUME | FX_HIDDEN | FX_SYSTEM)) == 0u)
        {
            UINT c = 0;
            while ((name[c] != '\0') && (c < (STORAGE_MAX_NAME_LEN - 1)))
            {
                file_names[n][c] = name[c];
                c++;
            }
            file_names[n][c] = '\0';
            file_sizes[n] = size;
            n++;
        }
        status = fx_directory_next_full_entry_find(&sdio_disk, name, &attributes,
                                                   &size, NULL, NULL, NULL, NULL, NULL, NULL);
    }
    file_count = n;
}

static VOID storage_thread_entry(ULONG input)
{
    ULONG flags;
    (void)input;

    for (;;)
    {
        /* Try to mount until a card is present and readable. */
        while (storage_state != STORAGE_MOUNTED)
        {
            if (fx_media_open(&sdio_disk, "PNP_SD", fx_stm32_sd_driver, 0,
                              fx_sd_media_memory, sizeof(fx_sd_media_memory)) == FX_SUCCESS)
            {
                scan_root_directory();
                storage_state = STORAGE_MOUNTED;
            }
            else
            {
                storage_state = STORAGE_NO_CARD;
                tx_thread_sleep(MOUNT_RETRY_TICKS);
            }
        }

        /* Mounted: service requests. */
        if (tx_event_flags_get(&storage_events,
                               EVT_RESCAN | EVT_USB_ATTACH | EVT_USB_DETACH,
                               TX_OR_CLEAR, &flags, TX_TIMER_TICKS_PER_SECOND) == TX_SUCCESS)
        {
            if (flags & EVT_USB_ATTACH)
            {
                /* Give the PC exclusive raw access: close our FAT view. */
                fx_media_close(&sdio_disk);
                storage_state = STORAGE_USB_ATTACHED;

                /* Wait until the USB side signals detach. */
                tx_event_flags_get(&storage_events, EVT_USB_DETACH,
                                   TX_OR_CLEAR, &flags, TX_WAIT_FOREVER);
                storage_state = STORAGE_NO_CARD;   /* forces remount + rescan */
            }
            else if (flags & EVT_RESCAN)
            {
                scan_root_directory();
            }
        }
        else
        {
            /* Periodic health check: a pulled card must drop the state. */
            CHAR volume[FX_MAX_SHORT_NAME_LEN];
            if (fx_media_volume_get(&sdio_disk, volume, FX_DIRECTORY_SECTOR) != FX_SUCCESS)
            {
                fx_media_close(&sdio_disk);
                storage_state = STORAGE_NO_CARD;
            }
        }
    }
}
