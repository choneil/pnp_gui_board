/**
  ******************************************************************************
  * @file    app_filex.h
  * @brief   Machine storage service: FileX on the microSD card (SDMMC2, CN13).
  *
  * Mounts the card in its own ThreadX thread and keeps a snapshot of the root
  * directory that the GUI can read at any time (lock-free single-writer
  * snapshot). Also provides the media hand-off points used later by the USB
  * MSC device (PC gets raw access while the firmware unmounts).
  ******************************************************************************
  */
#ifndef APP_FILEX_H
#define APP_FILEX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "fx_api.h"

/* Storage service state, readable from any thread. */
typedef enum
{
    STORAGE_NO_CARD = 0,   /* no card / mount failed, retrying   */
    STORAGE_MOUNTED,       /* FAT volume mounted, listing valid  */
    STORAGE_USB_ATTACHED   /* handed off to USB MSC (unmounted)  */
} storage_state_t;

#define STORAGE_MAX_FILES      24
#define STORAGE_MAX_NAME_LEN   48

/* Called once from tx_application_define with the app byte pool. */
UINT MX_FileX_Init(VOID *memory_ptr);

/* Snapshot accessors for the GUI (Model). */
storage_state_t storage_get_state(void);
UINT  storage_get_file_count(void);
/* Returns the ASCII name of entry i (0-based) or "" if out of range. */
const char* storage_get_file_name(UINT i);
ULONG storage_get_file_size(UINT i);
/* Request a directory re-scan on the storage thread. */
VOID  storage_request_rescan(void);

/* USB MSC hand-off hooks (used by the USBX device task later). */
VOID  storage_usb_attach(void);   /* close media, give card to USB   */
VOID  storage_usb_detach(void);   /* reclaim card, remount, rescan   */

#ifdef __cplusplus
}
#endif

#endif /* APP_FILEX_H */
