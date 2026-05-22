/*
 * main.c
 *
 * Description: Demonstrates Motion Detection.
 *
 * Motion Detection detects moving targets in the radar's field of view.
 *
 * Related Document: See README.md
 *
 ********************************************************************************
 * Copyright (c) (2016-2022), Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *******************************************************************************/

#include "cyhal.h"
#include "cy_retarget_io.h"
#include <stdbool.h>
#include "common.h"
#include "protocol.h"
#include "radar_motion_detection.h"
#include "resource_map.h"
#include "radar_config/indoor_motion_low_power.h"
#include "version.h"
#include "xensiv_bgt60cutrxx_mtb.h"
#include "xensiv_bgt60cutrxx_regs.h"


/*******************************************************************************
* Macros
*******************************************************************************/

/** Enables runtime measurement of data processing components */
//#define MEASURE_PROCESSING_TIME

/** baud rate for the UART communication
 * The baudrate must be high enough to transmit the data before the next interrupt occurs.
 */
#define UART_BAUDRATE 2000000

/** Default SPI speed for the XENSIV BGT60CUTRxx sensor, which is used during startup */
#define SPI_FREQUENCY               (18750000UL) // 18.75MHz

/*******************************************************************************
* Function Prototypes
*******************************************************************************/
static void xensiv_bgt60cutrxx_irq_handler(void* callback_arg, cyhal_gpio_event_t event);
static int32_t xensiv_bgt60cutrxx_board_init(void);
static int32_t initialize_board_leds(void);
static void timer_start(void);
static uint32_t timer_get_elapsed_us(void);
static void print_elapsed_time(uint32_t counter);

/*******************************************************************************
* Global Variables
*******************************************************************************/

/** Radar sensor object */
static xensiv_bgt60cutrxx_mtb_t sensor;

/** Motion Detection object */
static motion_detection_t motion_detection;

/** Buffer to store the full Range Doppler Map */
#ifdef RADAR_MOTION_DETECTION_USE_RD_BUFFER
static uint32_t rd_data_buffer[BGT60CUTR13_RD_DATABUFFER_MAX_SIZE];
#else
static uint32_t rd_data_buffer[0];
#endif

/** Helper variable used by IRQ handler to indicate data events to main loop */
static volatile bool radar_event_irq = false;

/** Helper variables used for runtime measurement */
#ifdef MEASURE_PROCESSING_TIME
static cyhal_timer_t timer_obj;
#endif
static uint32_t motion_detection_elapsed_us = 0;

/** Platform interface configuration */
static xensiv_bgt60cutrxx_mtb_iface_config_t mcu_iface_config =
{
    .selpin       = PIN_XENSIV_BGT60CUTRXX_SPI_CSN,
    .rstpin       = PIN_XENSIV_BGT60CUTRXX_RSTN,
    .irqpin       = PIN_XENSIV_BGT60CUTRXX_IRQ,
    .irq_priority = CYHAL_ISR_PRIORITY_DEFAULT,
    .spi_mosi     = CYBSP_SPI_MOSI,
    .spi_miso     = CYBSP_SPI_MISO,
    .spi_clk      = CYBSP_SPI_CLK,
    .spi_speed_hz = SPI_FREQUENCY,
};

/*******************************************************************************
* Function Definitions
*******************************************************************************/

/**
 * @brief Interrupt handler for the XENSIV BGT60CUTRxx radar sensor.
 *
 * This function is called when a radar interrupt occurs. It sets the radar_event_irq
 * flag to signal the main loop that radar data is ready for processing.
 *
 * @param[in] callback_arg User-defined callback argument (unused).
 * @param[in] event GPIO event that triggered the interrupt.
 */
static void xensiv_bgt60cutrxx_irq_handler(void* callback_arg, cyhal_gpio_event_t event)
{
    (void)callback_arg;
    (void)event;

    // flag to indicate radar event
    radar_event_irq = true;
}


/** Helper function to initialize board specific settings */
static int32_t xensiv_bgt60cutrxx_board_init(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /* Enable LDO */
    result |= cyhal_gpio_init(PIN_XENSIV_BGT60CUTRXX_LDO_EN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 1);

    /* Set specific pins to disable BGT60CUTRxx bootloader */
    result |= cyhal_gpio_init(PIN_XENSIV_BGT60CUTRXX_DIO2, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 1);
    result |= cyhal_gpio_init(PIN_XENSIV_BGT60CUTRXX_DIO3, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 1);

    /* Set level shifter direction */
    result |= cyhal_gpio_init(SHIELD_XENSIV_DIR4_EN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 1);

    if (result != CY_RSLT_SUCCESS)
    {
        return -1;
    }

    return XENSIV_BGT60CUTRXX_STATUS_OK;
}


/** Helper function to initialize board LEDs */
static int32_t initialize_board_leds(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    result |= cyhal_gpio_init(CYBSP_USER_LED, CYHAL_GPIO_DIR_OUTPUT,
                              CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_OFF);
    result |= cyhal_gpio_init(SHIELD_XENSIV_RADAR_LED_RED, CYHAL_GPIO_DIR_OUTPUT,
                              CYHAL_GPIO_DRIVE_STRONG, SHIELD_XENSIV_RADAR_LED_RED_OFF);
    result |= cyhal_gpio_init(SHIELD_XENSIV_RADAR_LED_GREEN, CYHAL_GPIO_DIR_OUTPUT,
                              CYHAL_GPIO_DRIVE_STRONG, SHIELD_XENSIV_RADAR_LED_GREEN_OFF);

    if (result != CY_RSLT_SUCCESS)
    {
        return -1;
    }

    return XENSIV_BGT60CUTRXX_STATUS_OK;
}


/** Helper function used for runtime measurement */
static void timer_start(void)
{
    #ifdef MEASURE_PROCESSING_TIME
    const cyhal_timer_cfg_t timer_cfg =
    {
        .compare_value = 0,                         // Timer compare value, not used
        .period        = CYHAL_TIMER_DEFAULT_FREQ,  // Timer period 1 second
        .direction     = CYHAL_TIMER_DIR_UP,        // Timer counts up
        .is_compare    = false,                     // Don't use compare mode
        .is_continuous = false,                     // Do not run timer indefinitely
        .value         = 0                          // Initial value of counter
    };

    cyhal_timer_free(&timer_obj);
    cyhal_timer_init(&timer_obj, NC, NULL);
    cyhal_timer_configure(&timer_obj, &timer_cfg);
    cyhal_timer_set_frequency(&timer_obj, CYHAL_TIMER_DEFAULT_FREQ);
    cyhal_timer_start(&timer_obj);
    #endif // ifdef MEASURE_PROCESSING_TIME
}


/** Helper function used for runtime measurement */
static uint32_t timer_get_elapsed_us(void)
{
    #ifdef MEASURE_PROCESSING_TIME
    cyhal_timer_stop(&timer_obj);
    return cyhal_timer_read(&timer_obj);
    #else
    return 0;
    #endif
}


/** Helper function used for runtime measurement */
static void print_elapsed_time(uint32_t counter)
{
    (void)counter;
    #ifdef MEASURE_PROCESSING_TIME
    print_algo_processing_times(counter, motion_detection_elapsed_us, 0, 0, Cy_SysClk_ClkHfGetFrequency(0UL));
    #endif
}


/**
 * @brief Main application entry point for radar motion detection code example.
 *
 * This function performs the following operations:
 * - Initializes the device, board peripherals, and UART for debug output
 * - Configures GPIO pins and SPI interface for the XENSIV BGT60CUTRxx radar sensor
 * - Sets up the radar sensor with interrupt-driven data acquisition
 * - Initializes motion detection with range-doppler and ultra-low-power sequences
 * - Enters an infinite loop processing radar interrupts, detecting motion

 *
 * @return int This function never returns under normal operation due to infinite loop.
 */
int main(void)
{
    cy_rslt_t result;

    #if defined (CY_DEVICE_SECURE)
    cyhal_wdt_t wdt_obj;

    /* Clear watchdog timer so that it doesn't trigger a reset */
    result = cyhal_wdt_init(&wdt_obj, cyhal_wdt_get_max_timeout_ms());
    CY_ASSERT(CY_RSLT_SUCCESS == result);
    cyhal_wdt_free(&wdt_obj);
    #endif /* #if defined (CY_DEVICE_SECURE) */

    // Initialize MCU and board peripherals
    result = cybsp_init();
    result |= initialize_board_leds();
    if (result != CY_RSLT_SUCCESS)
    {
        // MCU and board init failed. Stop program execution
        CY_ASSERT(0);
        return 1;
    }

    // Enable global interrupts
    __enable_irq();

    // Initialize UART port used for logging
    result = cy_retarget_io_init_fc(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, NC, NC, UART_BAUDRATE);
    if (result != CY_RSLT_SUCCESS)
    {
        // retarget-io init failed. Stop program execution
        CY_ASSERT(0);
        return 1;
    }

    // wait until UART is ready
    cyhal_system_delay_ms(5000);
    protocol_print_info("******************");
    protocol_print_info("RADAR: Motion Detection Example ");
    protocol_print_info("******************");

    protocol_print_info("For more projects, "
                        "visit our code examples repositories:");

    protocol_print_info("https://github.com/Infineon/"
                        "Code-Examples-for-ModusToolbox-Software");

    result = xensiv_bgt60cutrxx_board_init();
    result |= xensiv_bgt60cutrxx_platform_open(&sensor.iface, &mcu_iface_config);
    if (XENSIV_BGT60CUTRXX_STATUS_OK != result)
    {
        protocol_print_error("Error initializing platform interfaces");
        CY_ASSERT(0);
        return 1;
    }

    // Radar sensor initialization
    xensiv_bgt60cutrxx_power_mode_t power_mode = DEEP_SLEEP_MODE;
    bool deep_sleep_after_calibration = false;
    result = xensiv_bgt60cutrxx_platform_init(&sensor, &mcu_iface_config, register_list, ARRAY_SIZE(
                                                  register_list), power_mode, deep_sleep_after_calibration);
    if (XENSIV_BGT60CUTRXX_STATUS_OK != result)
    {
        protocol_print_error("Error initializing radar sensor");
        CY_ASSERT(0);
        return 1;
    }
    protocol_print_info("Radar sensor is initialized");

    result = xensiv_bgt60cutrxx_platform_irq_init(&sensor, &mcu_iface_config, &xensiv_bgt60cutrxx_irq_handler, NULL);
    if (XENSIV_BGT60CUTRXX_STATUS_OK != result)
    {
        protocol_print_error("Error initializing radar IRQ handler");
        CY_ASSERT(0);
        return 1;
    }
    protocol_print_info("Radar IRQ handler for motion detection events is enabled");

    #if MD_RD_ENABLED
    // Radar Motion Detection initialization
    const float rd_velocity_grid[] = RD_VELOCITY_GRID_MPS;
    const sequence_settings_t rd_sequence_settings =
    {
        .enabled            = true,
        .velocity_grid_mps  = rd_velocity_grid,
        .range_resolution_m = RD_RANGE_RESOLUTION_M
    };
    #else
    const sequence_settings_t rd_sequence_settings = {};
    #endif

    #if MD_ULP_ENABLED
    const float ulp_velocity_grid[] = ULP_VELOCITY_GRID_MPS;
    const sequence_settings_t ulp_sequence_settings =
    {
        .enabled            = true,
        .velocity_grid_mps  = ulp_velocity_grid,
        .range_resolution_m = ULP_RANGE_RESOLUTION_M
    };
    #else
    const sequence_settings_t ulp_sequence_settings = {};
    #endif

    result = radar_motion_detection_init(&motion_detection,
                                         &sensor.dev,
                                         rd_data_buffer,
                                         &rd_sequence_settings,
                                         &ulp_sequence_settings
                                         );
    if (XENSIV_BGT60CUTRXX_STATUS_OK != result)
    {
        protocol_print_error("Radar Motion Detection initialization failed");
        CY_ASSERT(0);
        return 1;
    }
    protocol_print_info("Radar Motion Detection is initialized");

    protocol_set_app_info(USE_CASE_NAME, VERSION, VERSION_BUILD);
    protocol_set_sensor_info("BGT60CUTR13B", VERSION, false,
                             sensor.dev.clk_freq_xtal == BGT60CUTRXX_CLK_80MHZ ? 80.0f : 40.0f);

    radar_event_irq = false;
    uint32_t irq_status = 0;
    xensiv_bgt60cutrxx_get_irq_status(motion_detection.radar_sensor, &irq_status);
    xensiv_bgt60cutrxx_set_irq_clear(motion_detection.radar_sensor, BGT60CUTR13B_IR_CLEAR_FOR_STARTUP_MSK);

    while (true)
    {
        static uint32_t counter = 0;

        protocol_handler(counter);

        if (radar_event_irq)
        {
            //clear internal interrupt flag
            radar_event_irq = false;

            // Process radar motion detection
            timer_start();
            int32_t md_result = radar_motion_detection_process(&motion_detection);
            motion_detection_elapsed_us = timer_get_elapsed_us();
            if (md_result < 0)
            {
                protocol_print_error("Motion Detection process error: %" PRId32, md_result);
                CY_ASSERT(0);
                return 1;
            }
            print_detections(&motion_detection, counter, true);
            print_elapsed_time(counter);
            counter++;
        }
    }
}


/* [] END OF FILE */
