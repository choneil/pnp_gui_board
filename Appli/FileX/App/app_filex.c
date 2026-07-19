/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_filex.c
  * @author  MCD Application Team
  * @brief   FileX applicative file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "app_filex.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "fx_stm32_sd_driver.h"
#include "ux_device_msc.h"
#include "app_camera.h"
#include "main.h"
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* Main thread stack size */
#define FX_APP_THREAD_STACK_SIZE         1024
/* Main thread priority */
#define FX_APP_THREAD_PRIO               10
/* USER CODE BEGIN PD */
/* fx_media_open plus a full directory scan needs far more than the CubeMX
   default 1 KB (FileX recurses and the long-name buffers are large). */
#undef  FX_APP_THREAD_STACK_SIZE
#define FX_APP_THREAD_STACK_SIZE  (4 * 1024)

/* How often the thread polls for card insertion / snapshot work. */
#define STORAGE_POLL_TICKS  10

/* Chunk used to stage snapshot bytes out of PSRAM into a DMA-friendly,
   32-byte aligned buffer in internal SRAM before handing them to FileX. */
#define STORAGE_BOUNCE_SIZE 4096
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* Main thread global data structures.  */
TX_THREAD       fx_app_thread;

/* USER CODE BEGIN PV */
static FX_MEDIA   sd_media;
static FX_FILE    sd_file;

/* FileX media working buffer. 32-byte aligned because the SD driver runs
   cache maintenance over it (FX_STM32_SD_CACHE_MAINTENANCE == 1). */
static UCHAR media_memory[4 * FX_STM32_SD_DEFAULT_SECTOR_SIZE] __attribute__((aligned(32)));
static UCHAR bounce[STORAGE_BOUNCE_SIZE] __attribute__((aligned(32)));

/* Published file table, read by the GUI thread under storage_mutex. */
typedef struct
{
  CHAR  name[STORAGE_NAME_LEN];
  ULONG size;
} storage_entry_t;

static storage_entry_t file_table[STORAGE_MAX_FILES];
static int             file_count = 0;
static TX_MUTEX        storage_mutex;

static volatile int  gui_state = 0;      /* mirrors storage_gui_state()     */
static volatile UINT rescan_requested = 1;
static UINT          media_opened = 0;

/* Scan scratch, kept off the thread stack (FileX long names are 256 bytes). */
static CHAR scan_name[FX_MAX_LONG_NAME_LEN];
static storage_entry_t scan_table[STORAGE_MAX_FILES];

/* --- Diagnostics, read live over SWD with STM32_Programmer_CLI -r32 --------
   Nothing reads these in firmware; they exist so a mount failure can be
   diagnosed on hardware without a debug session. See AFTER_CUBEMX_REGEN.md. */
volatile UINT  dbg_open_status   = 0xFFFFFFFFu;  /* last fx_media_open() return   */
volatile ULONG dbg_open_attempts = 0;            /* how many times it was tried   */
volatile UINT  dbg_drv_status    = 0xFFFFFFFFu;  /* media->fx_media_driver_status */
volatile UINT  dbg_drv_request   = 0xFFFFFFFFu;  /* last driver request issued    */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/

/* Main thread entry function.  */
void fx_app_thread_entry(ULONG thread_input);

/* USER CODE BEGIN PFP */
static UINT card_present(void);
static void publish_table(const storage_entry_t *src, int count);
static void scan_root_directory(void);
static void write_pending_snapshot(void);
/* USER CODE END PFP */

/**
  * @brief  Application FileX Initialization.
  * @param memory_ptr: memory pointer
  * @retval int
*/
UINT MX_FileX_Init(VOID *memory_ptr)
{
  UINT ret = FX_SUCCESS;
  TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL*)memory_ptr;
  VOID *pointer;

/* USER CODE BEGIN MX_FileX_MEM_POOL */

/* USER CODE END MX_FileX_MEM_POOL */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*Allocate memory for the main thread's stack*/
  ret = tx_byte_allocate(byte_pool, &pointer, FX_APP_THREAD_STACK_SIZE, TX_NO_WAIT);

/* Check FX_APP_THREAD_STACK_SIZE allocation*/
  if (ret != FX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }

/* Create the main thread.  */
  ret = tx_thread_create(&fx_app_thread, FX_APP_THREAD_NAME, fx_app_thread_entry, 0, pointer, FX_APP_THREAD_STACK_SIZE,
                         FX_APP_THREAD_PRIO, FX_APP_PREEMPTION_THRESHOLD, FX_APP_THREAD_TIME_SLICE, FX_APP_THREAD_AUTO_START);

/* Check main thread creation */
  if (ret != FX_SUCCESS)
  {
    return TX_THREAD_ERROR;
  }

/* USER CODE BEGIN MX_FileX_Init */
/* Do NOT create sd_tx_semaphore / sd_rx_semaphore here. The SD driver's
   FX_STM32_SD_PRE_INIT macro (Appli/FileX/Target/fx_stm32_sd_driver.h)
   creates them itself on FX_DRIVER_INIT, and ThreadX returns
   TX_SEMAPHORE_ERROR when a semaphore is created twice -- which makes that
   macro set FX_IO_ERROR, so fx_media_open fails at INIT and never reaches
   the boot sector read. That looks exactly like broken SD hardware. */
  if (tx_mutex_create(&storage_mutex, "storage table", TX_INHERIT) != TX_SUCCESS)
  {
    return TX_MUTEX_ERROR;
  }
/* USER CODE END MX_FileX_Init */

/* Initialize FileX.  */
  fx_system_initialize();

/* USER CODE BEGIN MX_FileX_Init 1*/

/* USER CODE END MX_FileX_Init 1*/

  return ret;
}

/**
 * @brief  Main thread entry.
 * @param thread_input: ULONG user argument used by the thread entry
 * @retval none
*/
 void fx_app_thread_entry(ULONG thread_input)
 {

/* USER CODE BEGIN fx_app_thread_entry 0*/
  FX_PARAMETER_NOT_USED(thread_input);
/* USER CODE END fx_app_thread_entry 0*/

/* USER CODE BEGIN fx_app_thread_entry 1*/
  for (;;)
  {
    /* While the PC owns the card over USB MSC, keep off the media entirely. */
    if (storage_get_state() == STORAGE_USB_ATTACHED)
    {
      gui_state = 2;
      tx_thread_sleep(STORAGE_POLL_TICKS);
      continue;
    }

    if (!card_present())
    {
      if (media_opened)
      {
        fx_media_close(&sd_media);
        media_opened = 0;
        publish_table(scan_table, 0);
      }
      gui_state = 0;
      /* Nothing can be written without a card; release the staging slot so
         the camera thread is not blocked from staging a later snapshot. */
      if (camera_snapshot_ready())
      {
        camera_snapshot_consumed();
      }
      rescan_requested = 1;   /* rescan whenever a card comes back */
      tx_thread_sleep(STORAGE_POLL_TICKS);
      continue;
    }

    if (!media_opened)
    {
      /* Do NOT touch hsd1 here: MX_SDMMC2_SD_Init runs lazily inside
         fx_media_open (via fx_stm32_sd_init on FX_DRIVER_INIT), so before the
         first open hsd1.Instance is still NULL and SDMMC2 is unclocked.
         Reading card state here faults and freezes every thread. */
      dbg_open_attempts++;
      dbg_open_status = fx_media_open(&sd_media, "PNP SD", fx_stm32_sd_driver, 0,
                                      media_memory, sizeof(media_memory));
      dbg_drv_status = sd_media.fx_media_driver_status;
      dbg_drv_request = sd_media.fx_media_driver_request;

      if (dbg_open_status != FX_SUCCESS)
      {
        /* A card is physically present but would not mount (unformatted, not
           FAT, or an IO error). Report that distinctly from "no card" so the
           two failures can be told apart on the panel. */
        gui_state = 3;
        tx_thread_sleep(STORAGE_POLL_TICKS);
        continue;
      }
      media_opened = 1;
      rescan_requested = 1;
    }

    gui_state = 1;

    /* A staged camera snapshot takes priority over listing. */
    if (camera_snapshot_ready())
    {
      write_pending_snapshot();
    }

    if (rescan_requested)
    {
      rescan_requested = 0;
      scan_root_directory();
    }

    tx_thread_sleep(STORAGE_POLL_TICKS);
  }
/* USER CODE END fx_app_thread_entry 1*/
  }

/* USER CODE BEGIN 1 */

/* SD_DETECT is active low on the STM32N6570-DK (card present pulls it down). */
static UINT card_present(void)
{
  return (HAL_GPIO_ReadPin(SD_DETECT_GPIO_Port, SD_DETECT_Pin) == GPIO_PIN_RESET) ? 1U : 0U;
}

/* Copy a freshly scanned table into the one the GUI reads. The mutex is held
   only for the memcpy, never across SD I/O, so the render thread cannot stall
   behind a card transfer. */
static void publish_table(const storage_entry_t *src, int count)
{
  if (count > STORAGE_MAX_FILES)
  {
    count = STORAGE_MAX_FILES;
  }

  if (tx_mutex_get(&storage_mutex, TX_WAIT_FOREVER) == TX_SUCCESS)
  {
    if (count > 0)
    {
      memcpy(file_table, src, (size_t)count * sizeof(storage_entry_t));
    }
    file_count = count;
    tx_mutex_put(&storage_mutex);
  }
}

/* Read the root directory into scan_table, skipping subdirectories. */
static void scan_root_directory(void)
{
  UINT  attributes = 0;
  ULONG size = 0;
  UINT  year, month, day, hour, minute, second;
  UINT  status;
  int   count = 0;

  status = fx_directory_first_full_entry_find(&sd_media, scan_name, &attributes, &size,
                                              &year, &month, &day, &hour, &minute, &second);

  while (status == FX_SUCCESS && count < STORAGE_MAX_FILES)
  {
    if ((attributes & (FX_DIRECTORY | FX_VOLUME)) == 0)
    {
      strncpy(scan_table[count].name, scan_name, STORAGE_NAME_LEN - 1);
      scan_table[count].name[STORAGE_NAME_LEN - 1] = '\0';
      scan_table[count].size = size;
      count++;
    }

    status = fx_directory_next_full_entry_find(&sd_media, scan_name, &attributes, &size,
                                               &year, &month, &day, &hour, &minute, &second);
  }

  publish_table(scan_table, count);
}

/* Write the BMP the camera thread staged in PSRAM, then release the slot.
   Bytes go out through an aligned SRAM bounce buffer so the SD DMA and its
   cache maintenance never operate directly on the PSRAM staging buffer. */
static void write_pending_snapshot(void)
{
  const uint8_t *data;
  uint32_t       length = 0;
  uint32_t       offset = 0;
  const char    *name = camera_snapshot_name();

  data = camera_snapshot_data(&length);
  if (data == 0 || length == 0)
  {
    camera_snapshot_consumed();
    return;
  }

  /* Overwrite any previous file of the same name. */
  fx_file_delete(&sd_media, (CHAR *)name);

  if (fx_file_create(&sd_media, (CHAR *)name) != FX_SUCCESS ||
      fx_file_open(&sd_media, &sd_file, (CHAR *)name, FX_OPEN_FOR_WRITE) != FX_SUCCESS)
  {
    camera_snapshot_consumed();
    return;
  }

  while (offset < length)
  {
    uint32_t chunk = length - offset;
    if (chunk > STORAGE_BOUNCE_SIZE)
    {
      chunk = STORAGE_BOUNCE_SIZE;
    }

    memcpy(bounce, data + offset, chunk);

    if (fx_file_write(&sd_file, bounce, chunk) != FX_SUCCESS)
    {
      break;
    }
    offset += chunk;
  }

  fx_file_close(&sd_file);
  fx_media_flush(&sd_media);

  camera_snapshot_consumed();
  rescan_requested = 1;   /* the new file should appear in the list */
}

/* ---- GUI-facing accessors (called from the TouchGFX thread) ------------- */

int storage_gui_state(void)
{
  return gui_state;
}

int storage_get_file_count(void)
{
  int count = 0;

  if (tx_mutex_get(&storage_mutex, TX_NO_WAIT) == TX_SUCCESS)
  {
    count = file_count;
    tx_mutex_put(&storage_mutex);
  }

  return count;
}

const char* storage_get_file_name(int index)
{
  const char *name = "";

  if (tx_mutex_get(&storage_mutex, TX_NO_WAIT) == TX_SUCCESS)
  {
    if (index >= 0 && index < file_count)
    {
      name = file_table[index].name;
    }
    tx_mutex_put(&storage_mutex);
  }

  return name;
}

uint32_t storage_get_file_size(int index)
{
  uint32_t size = 0;

  if (tx_mutex_get(&storage_mutex, TX_NO_WAIT) == TX_SUCCESS)
  {
    if (index >= 0 && index < file_count)
    {
      size = (uint32_t)file_table[index].size;
    }
    tx_mutex_put(&storage_mutex);
  }

  return size;
}

void storage_request_rescan(void)
{
  rescan_requested = 1;
}

/* USER CODE END 1 */
