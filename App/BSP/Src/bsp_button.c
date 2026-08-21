/**
 * @file    bsp_button.c
 * @brief   Button BSP implementation — two buttons, two responsibilities.
 *
 * PA0 (B1 on-board) — Menu Navigate:
 *   Short press only. Detects rising then falling edge, measures duration,
 *   fires BTN_EVENT_SHORT_PRESS if duration >= BUTTON_DEBOUNCE_MS.
 *   Long-press detection removed — PA0 is a single-gesture input.
 *
 * PE1 (external button) — Start / Confirm:
 *   Fires BTN_EVENT_LONG_PRESS on every falling edge (button press).
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
                    s_callback(BTN_EVENT_SHORT_PRESS);
                }
            }
            /* else: noise, ignore */
        }
    }
}

/* --------------------------------------------------------------------------
 * Public API — PE1 Start/Confirm button
 * -------------------------------------------------------------------------- */

void BSP_StartButton_Init(void)
{
    /*
     * Configure PE1 as falling-edge EXTI with internal pull-up.
     * The button connects PE1 to GND — pressing it drives the line LOW.
     * Only the falling edge (press moment) is detected; release is ignored.
     */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOE_CLK_ENABLE();
    GPIO_InitStruct.Pin  = START_BUTTON_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(START_BUTTON_PORT, &GPIO_InitStruct);

    /* EXTI1 — does not conflict with EXTI0 (PA0). */
    HAL_NVIC_SetPriority(EXTI1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI1_IRQn);
}

void BSP_StartButton_IRQHandler(void)
{
    /*
     * Every falling edge on PE1 is treated as a Start/Confirm action.
     * Fire BTN_EVENT_LONG_PRESS so UITask sends a command without any
     * code change — long press was always the "start" gesture.
     */
    if (s_callback != NULL)
    {
        s_callback(BTN_EVENT_LONG_PRESS);
    }
}
