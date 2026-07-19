# Read this after regenerating code from STM32N6570-DK.ioc

A CubeMX regeneration on **2026-07-17** silently reverted three pieces of
hand-written configuration. Each one produced a confusing hardware symptom that
looked like a driver or graphics bug, and together they cost most of a day to
re-diagnose on 2026-07-18. CubeMX only preserves `USER CODE` sections; anything
outside them is regenerated from the `.ioc`.

Most of the fixes below have since been **moved into `USER CODE` sections** so
they now survive regeneration. Verify them anyway — that migration is itself
only as durable as this file.

## 1. `TX_APP_MEM_POOL_SIZE` — panel shows a screenful of garbage

`Appli/AZURE_RTOS/App/app_azure_rtos_config.h`

CubeMX regenerates this as `8192`. Thread stacks drawn from the pool are
TouchGFX 4080 + camera 4096 + FileX 4096, so 8192 is not enough for even the
first two once ThreadX per-block overhead is counted. `MX_Camera_Init` then
fails, `App_ThreadX_Init` returns an error, and `tx_application_define` sits in
`while(1)`. The kernel never starts, so the LTDC scans uninitialised FB_RAM.

**The symptom is garbage pixels on the panel, not a blank screen or a visible
crash.** If you see that after a flash, check this constant first.

Now overridden to `32768` via `#undef` in `USER CODE BEGIN EC`.

`MX_Camera_Init` is also `(void)`-non-fatal in `Appli/Core/Src/app_threadx.c`
so a camera failure can never again prevent the GUI from booting.

## 2. SDMMC2 RIF master attributes — "SD CARD UNREADABLE" on a good card

`Appli/Core/Src/main.c`, `SystemIsolation_Config`

The generated body configures RIF masters for DCMIPP, DMA2D, GPU2D and LTDC1
but **not SDMMC2**. SDMMC2 only acts as a bus master when its IDMA moves sector
data into SRAM, so without it the card **enumerates perfectly** — `hsd1` shows
`State=READY`, `ErrorCode=0`, correct `BlockNbr`/`BlockSize` — while every data
transfer is blocked. `fx_media_open` cannot read the boot sector and the GUI
reports `SD CARD UNREADABLE` (`storage_gui_state() == 3`).

A healthy `hsd1` combined with a failing mount is a near-unambiguous signature
of missing bus-master permissions rather than a bad card or SD layer.

Now re-applied in `USER CODE BEGIN RIF_Init 1`.

## 3. `MX_DCMIPP_Init()` in `main()` — camera preview goes black

`Appli/Core/Src/main.c`

CubeMX generates its own `MX_DCMIPP_Init()` and calls it from the peripheral
init block. It drives the **same global `hdcmipp`** as the camera service but
configures PIPE0 / YUV420 / **80 Mbps** / **1 lane** / BPP6. The IMX335 needs
PIPE1 / RAW10 / **1600 Mbps** / **2 lanes** / BPP10. Running both leaves the
D-PHY configured for the wrong bitrate and no CSI data ever arrives — the
sensor still answers on I2C, so `IMX335_Probe` succeeds and `cam_state` reaches
`CAM_RUNNING` while the preview stays black.

The call is commented out; the camera service owns the DCMIPP. A guarded
`HAL_DCMIPP_DeInit` in `USER CODE BEGIN 2` returns the handle to `RESET` so the
camera thread reconfigures cleanly even if the call is ever restored.

## Not a regression, but do not "fix" it back

`Appli/Core/Src/stm32n6xx_hal_msp.c`, `HAL_DCMIPP_MspInit`

The camera clock dividers deliberately differ from ST's `DCMIPP_ContinuousMode`
example. That example runs **PLL1 at 1200 MHz**; this project's FSBL runs it at
**800 MHz** (HSE 48 / PLLM 6 * PLLN 100, P1=P2=1). ST's dividers of 4 and 60
would give 200 MHz and 13.33 MHz instead of the intended 300 MHz and 20 MHz.

A CSI D-PHY reference at 2/3 of expected makes the PHY lock near 1067 Mbps
while the sensor transmits 1600 Mbps: every line syncs on its start pattern
then loses bit alignment after ~14 pixels, giving a narrow white strip down the
left edge of the preview with correct 1600-byte row pitch across all 480 rows.

Correct values for PLL1 = 800 MHz: **IC17 = /3** (266 MHz) and **IC18 = /40**
(exactly 20 MHz). Recompute both if PLL1 ever changes; /2 on IC17 would be
400 MHz, above the peripheral maximum.

## Also check

- `Appli/FileX/App/app_filex.c` — the whole storage service lives in `USER CODE`
  sections here. It was found reset to the bare CubeMX skeleton (empty
  `fx_app_thread_entry`, no `fx_media_open` anywhere) and had to be rewritten
  from scratch. If storage stops working, check for an empty thread entry first.
- `FX_APP_THREAD_STACK_SIZE` is `#undef`'d to 4 KB in `USER CODE BEGIN PD` in
  that file; CubeMX's 1 KB default cannot survive `fx_media_open` plus a
  directory scan.
- `Appli/TouchGFX/gui/src/model/Model.cpp` — the board build must route the file
  API to `storage_*`; it was found reverted to hardcoded `STUB_FILES`.

## Reading a running board without a debug session

This is how the camera and SD faults were diagnosed. Non-destructive:

    STM32_Programmer_CLI.exe -c port=SWD mode=HOTPLUG -r32 <addr> <len>

`mode=HOTPLUG` matters — `NORMAL` resets the target and you lose the state you
wanted to inspect. Symbol addresses come from
`STM32CubeIDE/Appli/Debug/STM32N6570-DK_Appli.map` and **move on every build**,
so re-read the map rather than reusing old addresses.
