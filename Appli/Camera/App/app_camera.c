/**
  ******************************************************************************
  * @file    app_camera.c
  * @brief   IMX335 -> CSI-2 -> DCMIPP (downsize 2592x1944 -> 800x480 RGB565)
  *          -> ISP, streaming into a preview framebuffer. ThreadX-based.
  ******************************************************************************
  */
#include "app_camera.h"
#include "main.h"
#include <string.h>
#include "imx335.h"
#include "isp_api.h"
#include "imx335_E27_isp_param_conf.h"

/* ------------------------------------------------------------------ config */
#define CAM_THREAD_STACK   (4 * 1024)
#define CAM_THREAD_PRIO    12
#define IMX335_I2C_ADDRESS 0x34U

#define EVT_START          (1u << 0)
#define EVT_SNAPSHOT       (1u << 1)

/* Preview framebuffer: written by DCMIPP, read by the GUI. Placed in FB_RAM
   after the TouchGFX framebuffer (see the linker script's .camera_preview). */
ALIGN_32BYTES(static uint16_t camera_fb[CAM_WIDTH * CAM_HEIGHT])
    __attribute__((section(".camera_preview")));

/* Staged BMP for a snapshot, in external PSRAM (big + rarely touched). */
#define BMP_HEADER_SIZE 66
#define SNAP_MAX (BMP_HEADER_SIZE + CAM_WIDTH * CAM_HEIGHT * 2)
static uint8_t snapshot_buf[SNAP_MAX] __attribute__((section(".camera_snapshot")));
static uint32_t snapshot_len = 0;
static char snapshot_name[16];
static volatile UINT snapshot_ready = 0;
static uint32_t snapshot_index = 0;

/* ---------------------------------------------------------------- handles */
DCMIPP_HandleTypeDef hdcmipp;          /* referenced by the IRQ handlers */
static ISP_HandleTypeDef hcamera_isp;
static IMX335_Object_t IMX335Obj;
static I2C_HandleTypeDef hi2c1_cam;

static volatile camera_state_t cam_state = CAM_UNINIT;
static volatile uint32_t frame_id = 0;
static int32_t isp_gain, isp_exposure;

static TX_THREAD cam_thread;
static TX_EVENT_FLAGS_GROUP cam_events;

/* ----------------------------------------------------- I2C1 sensor bus IO */
static void MX_I2C1_Cam_Init(void)
{
    GPIO_InitTypeDef g = {0};
    RCC_PeriphCLKInitTypeDef pclk = {0};

    pclk.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
    pclk.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
    HAL_RCCEx_PeriphCLKConfig(&pclk);

    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();

    /* SCL = PH9, SDA = PC1, AF4 */
    g.Mode = GPIO_MODE_AF_OD;
    g.Pull = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF4_I2C1;
    g.Pin = GPIO_PIN_9;  HAL_GPIO_Init(GPIOH, &g);
    g.Pin = GPIO_PIN_1;  HAL_GPIO_Init(GPIOC, &g);

    hi2c1_cam.Instance = I2C1;
    hi2c1_cam.Init.Timing = 0x109035B7;   /* same as the board's I2C2 @ PCLK1 */
    hi2c1_cam.Init.OwnAddress1 = 0;
    hi2c1_cam.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1_cam.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1_cam.Init.OwnAddress2 = 0;
    hi2c1_cam.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c1_cam.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1_cam.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1_cam);
}

static int32_t cam_i2c_init(void)   { return 0; }
static int32_t cam_i2c_deinit(void) { return 0; }
static int32_t cam_i2c_gettick(void) { return (int32_t)HAL_GetTick(); }

static int32_t cam_i2c_write(uint16_t addr, uint16_t reg, uint8_t *p, uint16_t len)
{
    return (HAL_I2C_Mem_Write(&hi2c1_cam, addr, reg, I2C_MEMADD_SIZE_16BIT,
                              p, len, 1000) == HAL_OK) ? 0 : -1;
}
static int32_t cam_i2c_read(uint16_t addr, uint16_t reg, uint8_t *p, uint16_t len)
{
    return (HAL_I2C_Mem_Read(&hi2c1_cam, addr, reg, I2C_MEMADD_SIZE_16BIT,
                             p, len, 1000) == HAL_OK) ? 0 : -1;
}

static int32_t IMX335_Probe(void)
{
    IMX335_IO_t io;
    uint32_t id;

    io.Address = IMX335_I2C_ADDRESS;
    io.Init = cam_i2c_init;
    io.DeInit = cam_i2c_deinit;
    io.ReadReg = cam_i2c_read;
    io.WriteReg = cam_i2c_write;
    io.GetTick = cam_i2c_gettick;

    if (IMX335_RegisterBusIO(&IMX335Obj, &io) != IMX335_OK) { return -1; }
    if (IMX335_ReadID(&IMX335Obj, &id) != IMX335_OK) { return -1; }
    if (id != (uint32_t)IMX335_CHIP_ID) { return -1; }
    if (IMX335_Init(&IMX335Obj, IMX335_R2592_1944, IMX335_RAW_RGGB10) != IMX335_OK) { return -1; }
    if (IMX335_SetFrequency(&IMX335Obj, IMX335_INCK_24MHZ) != IMX335_OK) { return -1; }
    return 0;
}

/* ------------------------------------------------------------- ISP helpers */
static ISP_StatusTypeDef helpGetInfo(uint32_t i, ISP_SensorInfoTypeDef *info)
{ (void)i; return (ISP_StatusTypeDef)IMX335_GetSensorInfo(&IMX335Obj, (IMX335_SensorInfo_t*)info); }
static ISP_StatusTypeDef helpSetGain(uint32_t i, int32_t g)
{ (void)i; isp_gain = g; return (ISP_StatusTypeDef)IMX335_SetGain(&IMX335Obj, g); }
static ISP_StatusTypeDef helpGetGain(uint32_t i, int32_t *g)
{ (void)i; *g = isp_gain; return ISP_OK; }
static ISP_StatusTypeDef helpSetExp(uint32_t i, int32_t e)
{ (void)i; isp_exposure = e; return (ISP_StatusTypeDef)IMX335_SetExposure(&IMX335Obj, e); }
static ISP_StatusTypeDef helpGetExp(uint32_t i, int32_t *e)
{ (void)i; *e = isp_exposure; return ISP_OK; }

/* ---------------------------------------------------------- DCMIPP config */
static int32_t MX_DCMIPP_Init(void)
{
    DCMIPP_PipeConfTypeDef pipe = {0};
    DCMIPP_CSI_PIPE_ConfTypeDef csipipe = {0};
    DCMIPP_CSI_ConfTypeDef csi = {0};
    DCMIPP_DownsizeTypeDef ds = {0};

    hdcmipp.Instance = DCMIPP;
    if (HAL_DCMIPP_Init(&hdcmipp) != HAL_OK) { return -1; }

    csi.DataLaneMapping = DCMIPP_CSI_PHYSICAL_DATA_LANES;
    csi.NumberOfLanes = DCMIPP_CSI_TWO_DATA_LANES;
    csi.PHYBitrate = DCMIPP_CSI_PHY_BT_1600;
    if (HAL_DCMIPP_CSI_SetConfig(&hdcmipp, &csi) != HAL_OK) { return -1; }
    if (HAL_DCMIPP_CSI_SetVCConfig(&hdcmipp, DCMIPP_VIRTUAL_CHANNEL0, DCMIPP_CSI_DT_BPP10) != HAL_OK) { return -1; }

    csipipe.DataTypeMode = DCMIPP_DTMODE_DTIDA;
    csipipe.DataTypeIDA = DCMIPP_DT_RAW10;
    csipipe.DataTypeIDB = DCMIPP_DT_RAW10;
    if (HAL_DCMIPP_CSI_PIPE_SetConfig(&hdcmipp, DCMIPP_PIPE1, &csipipe) != HAL_OK) { return -1; }

    pipe.FrameRate = DCMIPP_FRAME_RATE_ALL;
    pipe.PixelPackerFormat = DCMIPP_PIXEL_PACKER_FORMAT_RGB565_1;
    pipe.PixelPipePitch = CAM_WIDTH * 2;
    if (HAL_DCMIPP_PIPE_SetConfig(&hdcmipp, DCMIPP_PIPE1, &pipe) != HAL_OK) { return -1; }

    /* Downsize 2592x1944 -> 800x480 (values from ST's continuous-mode example). */
    ds.HRatio = 25656; ds.VRatio = 33161;
    ds.HSize = CAM_WIDTH; ds.VSize = CAM_HEIGHT;
    ds.HDivFactor = 316; ds.VDivFactor = 253;
    if (HAL_DCMIPP_PIPE_SetDownsizeConfig(&hdcmipp, DCMIPP_PIPE1, &ds) != HAL_OK) { return -1; }
    if (HAL_DCMIPP_PIPE_EnableDownsize(&hdcmipp, DCMIPP_PIPE1) != HAL_OK) { return -1; }
    return 0;
}

/* ----------------------------------------------------- DCMIPP/ISP callbacks */
void HAL_DCMIPP_PIPE_FrameEventCallback(DCMIPP_HandleTypeDef *h, uint32_t Pipe)
{
    (void)h; (void)Pipe;
    frame_id++;
}

void HAL_DCMIPP_PIPE_VsyncEventCallback(DCMIPP_HandleTypeDef *h, uint32_t Pipe)
{
    (void)h;
    if (Pipe == DCMIPP_PIPE1)
    {
        ISP_IncMainFrameId(&hcamera_isp);
        ISP_GatherStatistics(&hcamera_isp);
    }
}

/* ------------------------------------------------------------ snapshot BMP */
static void stage_snapshot(void)
{
    /* Make the DMA-written frame visible to the CPU copy. */
    SCB_InvalidateDCache_by_Addr((uint32_t*)camera_fb, sizeof(camera_fb));

    uint8_t *h = snapshot_buf;
    uint32_t imgSize = CAM_WIDTH * CAM_HEIGHT * 2;
    uint32_t fileSize = BMP_HEADER_SIZE + imgSize;
    /* BITMAPFILEHEADER */
    h[0] = 'B'; h[1] = 'M';
    h[2] = fileSize; h[3] = fileSize >> 8; h[4] = fileSize >> 16; h[5] = fileSize >> 24;
    h[6] = h[7] = h[8] = h[9] = 0;
    h[10] = BMP_HEADER_SIZE; h[11] = h[12] = h[13] = 0;
    /* BITMAPINFOHEADER (40) */
    h[14] = 40; h[15] = h[16] = h[17] = 0;
    h[18] = CAM_WIDTH & 0xFF; h[19] = CAM_WIDTH >> 8; h[20] = h[21] = 0;
    h[22] = CAM_HEIGHT & 0xFF; h[23] = CAM_HEIGHT >> 8; h[24] = h[25] = 0;  /* +height = bottom-up */
    h[26] = 1; h[27] = 0;             /* planes */
    h[28] = 16; h[29] = 0;            /* bpp */
    h[30] = 3; h[31] = h[32] = h[33] = 0;  /* BI_BITFIELDS */
    h[34] = imgSize; h[35] = imgSize >> 8; h[36] = imgSize >> 16; h[37] = imgSize >> 24;
    for (int i = 38; i < 54; i++) { h[i] = 0; }
    /* colour masks: R 0xF800, G 0x07E0, B 0x001F */
    h[54] = 0x00; h[55] = 0xF8; h[56] = 0x00; h[57] = 0x00;
    h[58] = 0xE0; h[59] = 0x07; h[60] = 0x00; h[61] = 0x00;
    h[62] = 0x1F; h[63] = 0x00; h[64] = 0x00; h[65] = 0x00;

    /* pixel rows, bottom-up */
    uint8_t *dst = snapshot_buf + BMP_HEADER_SIZE;
    for (int row = CAM_HEIGHT - 1; row >= 0; row--)
    {
        const uint8_t *src = (const uint8_t*)&camera_fb[row * CAM_WIDTH];
        for (uint32_t b = 0; b < CAM_WIDTH * 2; b++) { *dst++ = src[b]; }
    }

    snapshot_len = fileSize;
    snapshot_index++;
    /* SNAP_%04lu.BMP */
    const char digits[] = "0123456789";
    uint32_t n = snapshot_index;
    snapshot_name[0]='S';snapshot_name[1]='N';snapshot_name[2]='A';snapshot_name[3]='P';
    snapshot_name[4]=digits[(n/1000)%10];snapshot_name[5]=digits[(n/100)%10];
    snapshot_name[6]=digits[(n/10)%10];snapshot_name[7]=digits[n%10];
    snapshot_name[8]='.';snapshot_name[9]='B';snapshot_name[10]='M';snapshot_name[11]='P';snapshot_name[12]=0;
    snapshot_ready = 1;
}

/* ---------------------------------------------------------------- thread */
static VOID cam_thread_entry(ULONG in)
{
    ULONG flags;
    (void)in;

    /* Block until the Camera screen asks us to start. */
    tx_event_flags_get(&cam_events, EVT_START, TX_OR_CLEAR, &flags, TX_WAIT_FOREVER);
    cam_state = CAM_STARTING;

    ISP_AppliHelpersTypeDef help = {0};
    help.GetSensorInfo = helpGetInfo;
    help.SetSensorGain = helpSetGain;
    help.GetSensorGain = helpGetGain;
    help.SetSensorExposure = helpSetExp;
    help.GetSensorExposure = helpGetExp;

    /* Blank the preview buffer before the DCMIPP starts filling it, so a
       partially written first frame never shows uninitialised memory. */
    memset(camera_fb, 0, sizeof(camera_fb));
    SCB_CleanDCache_by_Addr((uint32_t*)camera_fb, sizeof(camera_fb));

    MX_I2C1_Cam_Init();
    if (MX_DCMIPP_Init() != 0) { cam_state = CAM_ERROR; return; }
    if (IMX335_Probe() != 0) { cam_state = CAM_ERROR; return; }
    if (ISP_Init(&hcamera_isp, &hdcmipp, 0, &help, ISP_IQParamCacheInit[0]) != ISP_OK) { cam_state = CAM_ERROR; return; }
    if (HAL_DCMIPP_CSI_PIPE_Start(&hdcmipp, DCMIPP_PIPE1, DCMIPP_VIRTUAL_CHANNEL0,
                                  (uint32_t)camera_fb, DCMIPP_MODE_CONTINUOUS) != HAL_OK) { cam_state = CAM_ERROR; return; }
    if (ISP_Start(&hcamera_isp) != ISP_OK) { cam_state = CAM_ERROR; return; }

    cam_state = CAM_RUNNING;

    for (;;)
    {
        ISP_BackgroundProcess(&hcamera_isp);

        if (tx_event_flags_get(&cam_events, EVT_SNAPSHOT, TX_OR_CLEAR, &flags, TX_NO_WAIT) == TX_SUCCESS)
        {
            if (!snapshot_ready)
            {
                /* Sync to a frame boundary first: DCMIPP writes camera_fb
                   continuously, so starting the copy right after a completed
                   frame gives it a full frame period of head start and avoids
                   an obviously torn image. */
                uint32_t start = frame_id;
                uint32_t spins = 0;
                while (frame_id == start && spins++ < 100) { tx_thread_sleep(1); }
                stage_snapshot();
            }
        }
        tx_thread_sleep(1);
    }
}

/* ---------------------------------------------------------------- API */
UINT MX_Camera_Init(VOID *memory_ptr)
{
    TX_BYTE_POOL *pool = (TX_BYTE_POOL*)memory_ptr;
    VOID *stack;

    if (tx_event_flags_create(&cam_events, "cam events") != TX_SUCCESS) { return TX_GROUP_ERROR; }
    if (tx_byte_allocate(pool, &stack, CAM_THREAD_STACK, TX_NO_WAIT) != TX_SUCCESS) { return TX_POOL_ERROR; }
    return tx_thread_create(&cam_thread, "camera", cam_thread_entry, 0, stack,
                            CAM_THREAD_STACK, CAM_THREAD_PRIO, CAM_THREAD_PRIO,
                            TX_NO_TIME_SLICE, TX_AUTO_START);
}

VOID camera_request_start(void) { tx_event_flags_set(&cam_events, EVT_START, TX_OR); }
camera_state_t camera_get_state(void) { return cam_state; }
uint32_t camera_get_frame_id(void) { return frame_id; }

const uint16_t* camera_get_framebuffer(void)
{
    /* Until the pipeline is streaming, camera_fb holds uninitialised FB_RAM.
       Handing that to the GUI paints garbage, so report "no frame" instead. */
    if (cam_state != CAM_RUNNING)
    {
        return 0;
    }

    /* Ensure the CPU/GPU sees the latest DMA frame. */
    SCB_InvalidateDCache_by_Addr((uint32_t*)camera_fb, sizeof(camera_fb));
    return camera_fb;
}

VOID camera_request_snapshot(void) { tx_event_flags_set(&cam_events, EVT_SNAPSHOT, TX_OR); }
UINT camera_snapshot_ready(void) { return snapshot_ready; }
const uint8_t* camera_snapshot_data(uint32_t *length) { if (length) { *length = snapshot_len; } return snapshot_buf; }
const char* camera_snapshot_name(void) { return snapshot_name; }
VOID camera_snapshot_consumed(void) { snapshot_ready = 0; }
