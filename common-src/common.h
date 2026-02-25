/*
 * common.h
 *
 * Description: This header file contains the declarations and definitions for common functions used in all code
 * examples.
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

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <inttypes.h>
#include "radar_clustering.h"
#include "radar_motion_detection.h"
#include "radar_clutter_localization.h"
#include "xensiv_bgt60cutrxx.h"
/**
 * @brief Prints the presence status in JSON format.
 *
 * This function prints the presence status of the motion detection system
 * in a JSON-like format.
 *
 * This format can be read by the ifxapp visualization tool
 *
 * @param motion_detection Pointer to the motion detection structure.
 * @param presence The presence status (true/false).
 * @param counter The current timestamp or counter value.
 */
void print_presence(bool presence, uint32_t counter);

/** @brief Prints the motion detection targets in JSON format.
 *
 * This function iterates through the processed targets in the motion detection structure
 * and prints their azimuth, elevation, range, velocity, magnitude, and antenna index
 * in a JSON-like format.
 *
 * This format can be read by the ifxapp visualization tool
 *
 * @param motion_detection Pointer to the motion detection structure containing target data.
 * @param counter The current timestamp or counter value.
 */
void print_detections(motion_detection_t* motion_detection, uint32_t counter);

/** @brief Prints the clutter zones in JSON format.
 *
 * This function iterates through the defined clutter zones in the clutter localization structure
 * and prints the exclusion zones in a JSON-like format.
 * If no zones are defined, it prints empty arrays.
 * The zones are only printed when there is a change in the register update state, e.g. when the zones are set or when
 * the zones are reset.
 *
 * This format can be read by the ifxapp visualization tool
 *
 * @param clutter Pointer to the clutter localization structure containing clutter zone data.
 * @param counter The current timestamp or counter value.
 */
void print_clutter_zones(clutter_localization_t* clutter, uint32_t counter);


/** @brief Prints the calibration data in JSON format.
 *
 * This function prints the calibration settings and raw calibration data
 * in a JSON-like format.
 *
 * This format can be read by the ifxapp visualization tool
 *
 * @param calibration_settings Pointer to the calibration settings structure.
 * @param calibration_data Pointer to the array of raw calibration data.
 * @param counter The current timestamp or counter value.
 */
void print_calibration_data(xensiv_bgt60cutrxx_calibration_data_t* calibration_settings, uint32_t* calibration_data,
                            uint32_t counter);

/** @brief Prints the clustering results in JSON format.
 *
 * This function iterates through the clustering centroids
 * and prints their azimuth, elevation, range
 * in a JSON-like format.
 *
 * This format can be read by the ifxapp visualization tool
 *
 * @param clustering Pointer to the clustering structure containing data.
 * @param counter The current timestamp or counter value.
 */
void print_clustering_results(clustering_t* clustering, uint32_t counter);

/** @brief Prints the processing times of motion detection, clutter localization, and clustering algorithms.
 *
 * This function keeps track of the minimum, maximum, and average processing times
 * for motion detection, clutter localization, and clustering algorithms.
 * It prints these times in a JSON-like format.
 *
 * @param counter The current timestamp or counter value.
 * @param motion_detection_elapsed_us Elapsed time for motion detection in microseconds.
 * @param clutter_localization_elapsed_us Elapsed time for clutter localization in microseconds.
 * @param clustering_elapsed_us Elapsed time for clustering in microseconds.
 * @param clock_speed_hz The clock speed of the system in Hertz.
 */
void print_algo_processing_times(uint32_t counter, uint32_t motion_detection_elapsed_us,
                                 uint32_t clutter_localization_elapsed_us, uint32_t clustering_elapsed_us,
                                 uint32_t clock_speed_hz);

#endif // COMMON_H
