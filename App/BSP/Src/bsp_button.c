/**
 * @file    bsp_button.c
 * @brief   Button BSP implementation — gesture detection on PA0.
 */

#include "bsp_button.h"
#include "app_config.h"       /* BUTTON_DEBOUNCE_MS, BUTTON_LONG_PRESS_MS */
#include "stm32f4xx_hal.h"

#define BUTTON_PORT   GPIOA
#define BUTTON_PIN    GPIO_PIN_0

/* --------------------------------------------------------------------------
 * Module state
 * -------------------------------------------------------------------------- */

static BSP_Button_EventCb_t s_callback     = NULL;
static uint32_t             s_pressStartMs = 0u;
static uint8_t              s_pressed      = 0u;

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

void BSP_Button_Init(BSP_Button_EventCb_t callback)
{
    s_callback     = callback;
    s_pressStartMs = 0u;
    s_pressed      = 0u;

    /*
     * Reconfigure PA0 to detect both edges.
     * CubeMX generates GPIO_MODE_IT_RISING (rising only); we need rising AND
     * falling so we can measure the full press duration.
     */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin  = BUTTON_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(BUTTON_PORT, &GPIO_InitStruct);

    /*
     * Priority must be >= configMAX_SYSCALL_INTERRUPT_PRIORITY so that
     * xQueueSendFromISR() is safe to call from inside the registered callback.
     * The CubeMX-generated priority of 0 would bypass FreeRTOS masking and
     * cause a hard fault if any FromISR API is called.
     */
    HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

void BSP_Button_IRQHandler(void)
{
    if (HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN) == GPIO_PIN_SET)
    {
        /* Rising edge — record press start time. */
        s_pressStartMs = HAL_GetTick();
        s_pressed      = 1u;
    }
    else
    {
        /* Falling edge — measure duration and classify gesture. */
        if (s_pressed)
        {
            uint32_t duration = HAL_GetTick() - s_pressStartMs;
            s_pressed = 0u;

            if (duration < BUTTON_DEBOUNCE_MS)
            {
                /* Noise — ignore. */
            }
            else if (duration < BUTTON_LONG_PRESS_MS)
            {
                if (s_callback != NULL)
                {
                    s_callback(BTN_EVENT_SHORT_PRESS);
                }
            }
            else
            {
                if (s_callback != NULL)
                {
                    s_callback(BTN_EVENT_LONG_PRESS);
                }
            }
        }
    }
}
