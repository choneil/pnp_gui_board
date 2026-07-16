/**
  ******************************************************************************
  * @file    app_camera.h
  * @brief   IMX335 camera service: CSI-2 -> DCMIPP (downsize to 800x480 RGB565)
  *          -> ISP (auto exposure/white-balance), captured continuously into a
  *          preview framebuffer the GUI can display. Snapshot-to-SD is handed
  *          off to the FileX storage thread.
  ******************************************************************************
  */
#ifndef APP_CAMERA_H
#define APP_CAMERA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "tx_api.h"
#include <stdint.h>

#define CAM_WIDTH   800
#define CAM_HEIGHT  480

typedef enum
{
    CAM_UNINIT = 0,   /* not started yet                          */
    CAM_STARTING,     /* powering sensor / configuring pipeline   */
    CAM_RUNNING,      /* live frames landing in the preview buffer*/
    CAM_ERROR         /* sensor/pipeline init failed              */
} camera_state_t;

/* Create the camera thread; call once from tx_application_define. Non-fatal. */
UINT MX_Camera_Init(VOID *memory_ptr);

/* Ask the camera to power up and start streaming (idempotent). Call when the
   Camera screen opens; the sensor stays on afterwards. */
VOID camera_request_start(void);

camera_state_t camera_get_state(void);

/* Pointer to the live RGB565 preview buffer (CAM_WIDTH*CAM_HEIGHT), or 0. */
const uint16_t* camera_get_framebuffer(void);

/* Monotonic frame counter; the GUI compares it to detect a new frame. */
uint32_t camera_get_frame_id(void);

/* --- Snapshot hand-off (camera thread stages, storage thread writes) --- */
/* Ask for the next frame to be saved to SD. */
VOID camera_request_snapshot(void);
/* True when a staged BMP is waiting for the storage thread to write. */
UINT camera_snapshot_ready(void);
/* Staged BMP bytes + length, and the suggested filename. */
const uint8_t* camera_snapshot_data(uint32_t *length);
const char*    camera_snapshot_name(void);
/* Called by the storage thread after writing (frees the staging slot). */
VOID camera_snapshot_consumed(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_CAMERA_H */
