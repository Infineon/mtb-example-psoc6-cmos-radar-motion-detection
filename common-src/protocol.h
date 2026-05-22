#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

/*******************************************************************************
* Macros
*******************************************************************************/

/** Sync byte for frame start */
#define PROTOCOL_SYNC_BYTE          0xD9

/** Minimum frame size: sync + cmd + len(2) + crc(2) */
#define PROTOCOL_MIN_FRAME_SIZE     6

/** Maximum payload size (adjust as needed) */
#define PROTOCOL_MAX_PAYLOAD_SIZE   256

/** Circular buffer size (must be power of 2 for efficiency) */
#define PROTOCOL_BUFFER_SIZE        512

/*******************************************************************************
* Types
*******************************************************************************/
/* Terminal command types */
typedef enum
{
    CMD_EMPTY = 0,

    /**
     * @brief Get information about the system. Firmware version, sensor information, general system information.
     */
    CMD_GET_INFO = 1
} protocol_cmd_t;

/** Protocol parser states */
typedef enum
{
    PROTOCOL_STATE_WAIT_SYNC,       /**< Waiting for sync byte */
    PROTOCOL_STATE_WAIT_CMD,        /**< Waiting for command ID */
    PROTOCOL_STATE_WAIT_LEN_LOW,    /**< Waiting for length low byte */
    PROTOCOL_STATE_WAIT_LEN_HIGH,   /**< Waiting for length high byte */
    PROTOCOL_STATE_WAIT_PAYLOAD,    /**< Waiting for payload bytes */
    PROTOCOL_STATE_WAIT_CRC_LOW,    /**< Waiting for CRC low byte */
    PROTOCOL_STATE_WAIT_CRC_HIGH    /**< Waiting for CRC high byte */
} protocol_state_t;

/** Command frame structure */
typedef struct
{
    uint8_t  sync;                              /**< Sync byte (0xD9) */
    uint8_t  cmd_id;                            /**< Command ID */
    uint16_t payload_length;                    /**< Payload length */
    uint8_t  payload[PROTOCOL_MAX_PAYLOAD_SIZE]; /**< Payload data */
    uint16_t crc;                               /**< CRC checksum */
} protocol_message_t;

/*******************************************************************************
* Function Prototypes
*******************************************************************************/

/**
 * @brief Sets the application information.
 *
 * This function sets the application information including the application name,
 * version, and build.
 *
 * @param app_name The name of the application.
 * @param version The version of the application.
 * @param build The build identifier of the application.
 */
void protocol_set_app_info(const char* app_name, const char* version, const char* build);

/**
 * @brief Sets the sensor information.
 *
 * This function sets the sensor information including the sensor name,
 * driver version, configurability, and crystal frequency.
 *
 * @param sensor_name The name of the sensor.
 * @param driver_version The version of the sensor driver.
 * @param is_configurable Indicates if the sensor is configurable.
 * @param xtal_mhz The crystal frequency in MHz.
 */
void protocol_set_sensor_info(const char* sensor_name, const char* driver_version, bool is_configurable,
                              float xtal_mhz);

/**
 * @brief Sets the zoning configuration.
 *
 * This function sets the zoning configuration for the algorithm.
 *
 * @param config Pointer to the zoning configuration structure.
 */
void protocol_set_zone_config(const algo_zoning_event_t* config);

/**
 * @brief Prints informational messages in a structured format.
 *
 * This function takes a formatted string and its arguments, formats the message,
 * and prints it in a JSON-like structure for easy parsing on the receiving end.
 *
 * @param format The format string (printf-style).
 * @param ... Additional arguments for the format string.
 */
void protocol_print_info(const char* format, ...);

/**
 * @brief Prints error messages in a structured format.
 *
 * This function takes a formatted string and its arguments, formats the message,
 * and prints it in a JSON-like structure for easy parsing on the receiving end.
 *
 * @param format The format string (printf-style).
 * @param ... Additional arguments for the format string.
 */
void protocol_print_error(const char* format, ...);

/**
 * @brief Handles the protocol operations.
 *
 * This function processes the protocol tasks and should be called
 * periodically (e.g., in the main loop) to ensure proper protocol operation.
 * It reads data from the serial port, parses incoming frames, and dispatches
 * complete frames to the command handler.
 *
 * This function is non-blocking and maintains internal state between calls.
 * @param counter The current timestamp or counter value.
 */
void protocol_handler(uint32_t counter);
#endif // PROTOCOL_H
