
/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_azure_rtos_config.h
  * @author  MCD Application Team
  * @brief   app_azure_rtos config header file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef APP_AZURE_RTOS_CONFIG_H
#define APP_AZURE_RTOS_CONFIG_H
#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* Using static memory allocation via threadX Byte memory pools */

#define USE_STATIC_ALLOCATION                    1

#define TX_APP_MEM_POOL_SIZE                     8192

#define FX_APP_MEM_POOL_SIZE                     1024

#define UX_APP_MEM_POOL_SIZE                     1024

#define TOUCHGFX_APP_MEM_POOL_SIZE               4096

/* USER CODE BEGIN EC */

/* Thread stacks come out of this pool: TouchGFX 4080 + camera 4096 + FileX
   4096, plus ThreadX per-block overhead. CubeMX's default of 8192 above is
   not enough for even the first two, and the allocation failure hangs
   tx_application_define in while(1) -- the LTDC then scans uninitialised
   FB_RAM, so the panel shows a screenful of garbage rather than crashing.
   Overridden HERE, inside USER CODE, because a CubeMX regeneration on
   2026-07-17 silently reset the generated value and cost a debug session. */
#undef  TX_APP_MEM_POOL_SIZE
#define TX_APP_MEM_POOL_SIZE                     32768

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

#ifdef __cplusplus
}
#endif
#endif /* APP_AZURE_RTOS_CONFIG_H */
