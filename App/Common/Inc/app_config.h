/**
 * @file    app_config.h
 * @brief   Application-wide configuration constants.
 *
 * Central place for all timing, priority, and capacity values.
 * Change values here only — never scatter magic numbers across tasks.
 *
 * No FreeRTOS or hardware includes. Safe to include from any layer.
 */

#ifndef APP_COMMON_APP_CONFIG_H
#define APP_COMMON_APP_CONFIG_H

/* --------------------------------------------------------------------------
 * Simulation timing (milliseconds)
 * -------------------------------------------------------------------------- */

/** Duration of the water-fill phase. */
#define WASH_FILL_DURATION_MS       10000U

/** Duration of the active wash / agitation phase. */
#define WASH_CYCLE_DURATION_MS      20000U

/** Duration of the drain phase. */
#define WASH_DRAIN_DURATION_MS       5000U

/** Duration of the spin-only cycle. */
#define SPIN_DURATION_MS            10000U

/* --------------------------------------------------------------------------
 * Button behavior (milliseconds)
 * -------------------------------------------------------------------------- */

/** Hold time that distinguishes a long press from a short press. */
#define BUTTON_LONG_PRESS_MS         1500U

/** Debounce filter window applied in the ISR / BSP layer. */
#define BUTTON_DEBOUNCE_MS             50U

/* --------------------------------------------------------------------------
 * Queue capacities (number of items)
 * -------------------------------------------------------------------------- */

/** Button ISR  →  UITask queue depth. */
#define BUTTON_QUEUE_LENGTH              5

/** UITask  →  WashingManager queue depth. */
#define COMMAND_QUEUE_LENGTH             5

/** WashingManager  →  OLEDTask queue depth. */
#define DISPLAY_QUEUE_LENGTH             5

/* --------------------------------------------------------------------------
 * Task priorities
 *
 * configMAX_PRIORITIES = 5  (0 = lowest, 4 = highest).
 * Priority 0 is reserved for the Idle task.
 * -------------------------------------------------------------------------- */

/** OLEDTask — lowest application priority; pure renderer. */
#define TASK_PRIORITY_OLED               1

/** UITask — user input and menu logic. */
#define TASK_PRIORITY_UI                 2

/** SensorTask — periodic hardware polling. */
#define TASK_PRIORITY_SENSOR             2

/** WashingManager — state machine owner; highest application priority. */
#define TASK_PRIORITY_WASH_MANAGER       3

/* --------------------------------------------------------------------------
 * Sensor task
 * -------------------------------------------------------------------------- */

/** SensorTask polling period in milliseconds. */
#define SENSOR_TASK_PERIOD_MS          100U

#endif /* APP_COMMON_APP_CONFIG_H */
