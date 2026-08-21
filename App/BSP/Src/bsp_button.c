/**
 * @file    bsp_button.c
 * @brief   Button BSP implementation — two buttons, two responsibilities.
 *
 * PA0 (B1 on-board) — Menu Navigate:
 *   Short press only. Detects rising then falling edge, measures duration,
 *   fires BTN_EVENT_NAVIGATE if duration >= BUTTON_DEBOUNCE_MS.
 *   Long-press detection removed — PA0 is a single-gesture input.
 *
 *   Fires BTN_EVENT_CONFIRM on every falling edge (button press).
 *   No duration measurement. Internal pull-up — button connects PE1 to GND.
 *   Uses EXTI1 (no conflict with EXTI0 on PA0).
 */

#include "bsp_button.h"
#include "app_config.h"       /* BUTTON_DEBOUNCE_MS */
#include "stm32f4xx_hal.h"

#define MENU_BUTTON_PORT   GPIOA
#define MENU_BUTTON_PIN    GPIO_PIN_0

#define START_BUTTON_PORT  GPIOE
#define START_BUTTON_PIN   GPIO_PIN_1

/* --------------------------------------------------------------------------
 * Module state
 * -------------------------------------------------------------------------- */

static BSP_Button_EventCb_t s_callback     = NULL;
static uint32_t             s_pressStartMs = 0u;
static uint8_t              s_pressed      = 0u;

/* --------------------------------------------------------------------------
 * Public API — PA0 Menu Navigate button
 * -------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------
 * Public API — PA0 Menu Navigate button
 * --------------------------------------------------------------------------
 *
 * STM32CubeIDE / CubeMX configuration (.ioc):
 *   PA0 đã được CubeMX tự động tạo trong MX_GPIO_Init() (Freertos.ioc).
 *   Không cần chỉnh tay. Cài đặt gốc trong .ioc:
 *
 *     Pinout & Configuration → PA0
 *       GPIO mode   : External Interrupt Mode with Rising edge trigger
 *       GPIO Pull   : No pull-up and no pull-down
 *                     (board có pull-down ngoại vi — không cần pull nội)
 *       User Label  : B1 [Blue PushButton]
 *
 *     System Core → NVIC:
 *       EXTI line0 interrupt : Enabled
 *       Preemption Priority  : 0  (chú ý: BSP_Button_Init() ghi đè thành 5
 *                                   bên dưới để an toàn với FreeRTOS API)
 *
 * Sau khi gọ mã: BSP_Button_Init() tái cấu hình lại PA0 sang
 * RISING_FALLING (hai cạnh) để đo được thời gian nhấn.
 * -------------------------------------------------------------------------- */

void BSP_Button_Init(BSP_Button_EventCb_t callback)
{
    s_callback     = callback;
    s_pressStartMs = 0u;
    s_pressed      = 0u;

    /*
     * Reconfigure PA0 to detect both edges so we can measure press duration
     * for debounce filtering. CubeMX generates rising-only by default.
     */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin  = MENU_BUTTON_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;    /* DISCO board has external pull-down */
    HAL_GPIO_Init(MENU_BUTTON_PORT, &GPIO_InitStruct);

    /*
     * Priority must be >= configMAX_SYSCALL_INTERRUPT_PRIORITY (5) so that
     * xQueueSendFromISR() is safe to call from the callback.
     */
    HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

void BSP_Button_IRQHandler(void)
{
    if (HAL_GPIO_ReadPin(MENU_BUTTON_PORT, MENU_BUTTON_PIN) == GPIO_PIN_SET)
    {
        /* Rising edge — record press start time. */
        s_pressStartMs = HAL_GetTick();
        s_pressed      = 1u;
    }
    else
    {
        /* Falling edge — check duration for debounce, then fire short press. */
        if (s_pressed)
        {
            uint32_t duration = HAL_GetTick() - s_pressStartMs;
            s_pressed = 0u;

            if (duration >= BUTTON_DEBOUNCE_MS)
            {
                /* Any press longer than debounce threshold = short press.
                 * Long-press gesture is no longer handled on PA0. */
                if (s_callback != NULL)
                {
                    s_callback(BTN_EVENT_NAVIGATE);
                }
            }
            /* else: noise, ignore */
        }
    }
}

/* --------------------------------------------------------------------------
 * Public API — PE1 Start/Confirm button
 * -------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------
 * Public API — PE1 Start/Confirm button
 * --------------------------------------------------------------------------
 *
 * STM32CubeIDE / CubeMX configuration (.ioc):
 *   PE1 được cấu hình trong MX_GPIO_Init() (Freertos.ioc):
 *     PE1 : GPIO_MODE_IT_FALLING, Pull-up  (START_BTN)
 *   __HAL_RCC_GPIOE_CLK_ENABLE() và HAL_GPIO_Init() đã được CubeMX tạo.
 *
 *   BSP_StartButton_Init() chỉ cần:
 *     1. Set NVIC priority = 5 (CubeMX mặc định là 0 — không an toàn với FreeRTOS)
 *     2. Enable EXTI1 IRQ
 *
 * Kết nối phần cứng:
 *   MCU PE1 ─── [Nút Start] ─── GND  (pull-up nội được bật bởi CubeMX)
 * -------------------------------------------------------------------------- */

void BSP_StartButton_Init(void)
{
    /*
     * GPIO init for PE1 (EXTI falling edge, pull-up) is handled by
     * MX_GPIO_Init() (CubeMX generated). No GPIO init needed here.
     *
     * CubeMX does NOT enable EXTI1 IRQ in MX_GPIO_Init() — only PA0 (EXTI0)
     * is in the NVIC section of the .ioc. So we must enable EXTI1 here and
     * override the priority to 5 (FreeRTOS-safe).
     */
    HAL_NVIC_SetPriority(EXTI1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI1_IRQn);
}

void BSP_StartButton_IRQHandler(void)
{
    /*
     * Every falling edge on PE1 is treated as a Start/Confirm action.
     * Fire BTN_EVENT_CONFIRM so UITask sends a command without any
     * code change.
     */
    if (s_callback != NULL)
    {
        s_callback(BTN_EVENT_CONFIRM);
    }
}
