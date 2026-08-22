/**
 * @file    bsp_sensor.c
 * @brief   Sensor BSP implementation — door and water-level inputs simulated
 *          by physical push-buttons on PD5 (door) and PD6 (water).
 *
 * Simulation strategy:
 *   Both sensors are represented by static volatile flags.
 *   BSP_Sensor_Poll() is called periodically from SensorTask (every 100 ms).
 *   It reads PD5 and PD6, detects falling edges (button pressed), and
 *   toggles the corresponding flag.
 *
 *   Falling-edge detection is used instead of level reading so that a single
 *   button press produces exactly one toggle, regardless of how long the
 *   button is held.
 *
 * Why polling, not EXTI?
 *   PD5 uses EXTI5 and PD6 uses EXTI6 — neither conflicts with PA0 (EXTI0).
 *   Polling in SensorTask at 100 ms is chosen for simplicity: no ISR
 *   complexity, no NVIC priority concerns, and consistent with the task's
 *   existing sensor-reading role.
 *
 * Default states:
 *   s_doorClosed = 1  (closed)   — machine starts in a safe state.
 *   s_waterFull  = 0  (empty)    — machine must wait for water to fill.
 */

#include "bsp_sensor.h"
#include "stm32f4xx_hal.h"

/* --------------------------------------------------------------------------
 * Hardware constants
 * -------------------------------------------------------------------------- */

#define DOOR_PORT   GPIOD
#define DOOR_PIN    GPIO_PIN_5

#define WATER_PORT  GPIOD
#define WATER_PIN   GPIO_PIN_6

/* --------------------------------------------------------------------------
 * Module state
 * -------------------------------------------------------------------------- */

static volatile uint8_t s_doorClosed  = 1u;  /* 1 = closed, 0 = open        */
static volatile uint8_t s_waterFull   = 0u;  /* 1 = full,   0 = empty       */

/* Previous raw GPIO readings — used for falling-edge detection. */
static uint8_t s_prevDoorPin  = 1u;          /* pull-up default: pin is HIGH */
static uint8_t s_prevWaterPin = 1u;

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------------
 *
 * STM32CubeIDE / CubeMX configuration (.ioc):
 *   PD5 và PD6 được cấu hình trong MX_GPIO_Init() (Freertos.ioc):
 *     PD5 : GPIO Input, Pull-up  (DOOR_BTN)
 *     PD6 : GPIO Input, Pull-up  (WATER_BTN)
 *   __HAL_RCC_GPIOD_CLK_ENABLE() và HAL_GPIO_Init() đã được CubeMX tạo
 *   tự động — BSP_Sensor_Init() không cần khởi tạo GPIO nữa.
 *
 * Kết nối phần cứng:
 *   MCU PD5 ─── [Nút Door]  ─── GND  (pull-up nội được bật bởi CubeMX)
 *   MCU PD6 ─── [Nút Water] ─── GND  (pull-up nội được bật bởi CubeMX)
 * -------------------------------------------------------------------------- */

void BSP_Sensor_Init(void)
{
    /*
     * GPIO init for PD5 and PD6 is handled by MX_GPIO_Init() (CubeMX generated).
     * No additional hardware configuration needed here.
     * BSP_Sensor_Init() is kept as a placeholder for future non-GPIO init
     * (e.g. software state reset, calibration).
     */
}

void BSP_Sensor_Poll(void)
{
    /* Read current pin state: GPIO_PIN_SET = 1 (not pressed), RESET = 0 (pressed) */
    uint8_t doorPin  = (HAL_GPIO_ReadPin(DOOR_PORT,  DOOR_PIN)  == GPIO_PIN_SET) ? 1u : 0u;
    uint8_t waterPin = (HAL_GPIO_ReadPin(WATER_PORT, WATER_PIN) == GPIO_PIN_SET) ? 1u : 0u;

    /*
     * Falling edge on door pin: pin was HIGH (not pressed), now LOW (pressed).
     * Toggle the door flag once per press.
     */
    if ((s_prevDoorPin == 1u) && (doorPin == 0u))
    {
        s_doorClosed ^= 1u;
    }

    /*
     * Falling edge on water pin: pin was HIGH (not pressed), now LOW (pressed).
     * Toggle the water flag once per press.
     */
    if ((s_prevWaterPin == 1u) && (waterPin == 0u))
    {
        s_waterFull ^= 1u;
    }

    /* Save readings for next call */
    s_prevDoorPin  = doorPin;
    s_prevWaterPin = waterPin;
}

uint8_t BSP_Sensor_IsDoorClosed(void)
{
    return s_doorClosed;
}

uint8_t BSP_Sensor_IsWaterFull(void)
{
    return s_waterFull;
}
