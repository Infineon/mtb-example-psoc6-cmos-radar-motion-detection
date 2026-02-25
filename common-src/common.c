/*
 * common.c
 *
 * Description: This source file contains the implementations for common functions used in all code examples.
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

#include "common.h"
#include "xensiv_bgt60cutrxx_regs.h"


void print_presence(bool presence, uint32_t counter)
{
    // Print the presence status in JSON format
    //  e.g. {"presence": {"state": true}}
    printf("{\"ts\": %" PRIu32 ", \"presence\":{\"state\":%s}}\n",
           counter,
           (presence != false) ? "true" : "false");
}


void print_detections(motion_detection_t* motion_detection, uint32_t counter)
{
    uint16_t type = 0;
    bool target_detected = false;

    printf("{\"ts\": %" PRIu32 ", \"detections\": {", counter);
    printf(
        "\"attributes\":[\"range_m\",\"azimuth_deg\",\"elevation_deg\",\"magnitude_dBFS\",\"speed_mps\",\"rx_idx\",\"range_bin\",\"doppler_bin\",\"sequence_num\",\"type\",\"seq_count_1\",\"seq_count_2\",\"ir_stat\"], \"data\": [");

    if (motion_detection->seq1_done)
    {
        type = 1; // ulp detected
    }
    else if (motion_detection->seq2_done)
    {
        type = 2; // rd detected
    }
    else
    {
        type = 0; // No motion detected
    }

    for (uint8_t i = 0; i < BGT60CUTR13_PROCESSED_MD_TARGET_MAX; i++)
    {
        // check if targets are detected then print the targets and in case of no targets are detected, print an empty
        // array
        if ((motion_detection->processed_targets[i].antenna_rx > 0.0f) ||
            ((i == (BGT60CUTR13_PROCESSED_MD_TARGET_MAX - 1)) && !target_detected))
        {
            if (target_detected)
            {
                printf(", ");
            }
            target_detected = true;
            printf("[%.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %d, %d, %d, %d, %d, %d, %" PRIu32 "]",
                   motion_detection->processed_targets[i].range_m,
                   motion_detection->processed_targets[i].azimuth_deg,
                   motion_detection->processed_targets[i].elevation_deg,
                   motion_detection->processed_targets[i].magnitude_dBFS,
                   motion_detection->processed_targets[i].velocity_mps,
                   motion_detection->processed_targets[i].antenna_rx,
                   motion_detection->range_bin[i],
                   motion_detection->doppler_bin[i],
                   motion_detection->sequence,
                   type,
                   motion_detection->sequence_counters.seq1_counter,
                   motion_detection->sequence_counters.seq2_counter,
                   motion_detection->ir_stat);
        }
    }
    printf("]}}\n");
}


void print_clutter_zones(clutter_localization_t* clutter, uint32_t counter)
{
    static clutter_localization_state_t previous_state = CLUTTER_LOCALIZATION_STATE_IDLE;
    bool target_detected = false;


    //only print the zones if the exclusion zone registers are written
    if (clutter->register_update_state == previous_state)
    {
        return;
    }

    printf("{\"ts\": %" PRIu32 ", \"zones\": {", counter);

    // print only the plane which has been written to the sensor
    if (clutter->MD_ZONE_CTRL ==
        (BGT60CUTR13B_MD_ZONE_CTRL_PLANE_SEL_E_PLANE << BGT60CUTR13B_MD_ZONE_CTRL_PLANE_SEL_pos))
    {
        printf("\"azimuths\": [");
        for (uint8_t r_zone = 0; r_zone < CLUTTER_LOCALIZATION_NUM_RANGE_ZONES; r_zone++)
        {
            for (uint8_t a_zone = 0; a_zone < CLUTTER_LOCALIZATION_NUM_ANGLE_ZONES; a_zone++)
            {
                if (clutter->zone_marker_e_neighboring[r_zone][a_zone] >= CLUTTER_LOCALIZATION_EXCLUSION_VALUE)
                {
                    if (target_detected)
                    {
                        printf(", ");
                    }
                    target_detected = true;
                    printf("[%d, %d]", r_zone, a_zone);
                }
            }
        }
        // print empty elevations array
        printf("], \"elevations\": []}");
    }
    else
    {
        target_detected = false;
        printf("\"elevations\": [");
        for (uint8_t r_zone = 0; r_zone < CLUTTER_LOCALIZATION_NUM_RANGE_ZONES; r_zone++)
        {
            for (uint8_t a_zone = 0; a_zone < CLUTTER_LOCALIZATION_NUM_ANGLE_ZONES; a_zone++)
            {
                if (clutter->zone_marker_h_neighboring[r_zone][a_zone] >= CLUTTER_LOCALIZATION_EXCLUSION_VALUE)
                {
                    if (target_detected)
                    {
                        printf(", ");
                    }
                    target_detected = true;
                    printf("[%d, %d]", r_zone, a_zone);
                }
            }
        }
        // print empty azimuths array
        printf("], \"azimuths\": []}");
    }

    printf("}\n");
    previous_state = clutter->register_update_state;
}


void print_calibration_data(xensiv_bgt60cutrxx_calibration_data_t* calibration_settings, uint32_t* calibration_data,
                            uint32_t counter)
{
    if ((calibration_settings == NULL) || (calibration_data == NULL))
    {
        return;
    }

    printf("{\"ts\": %" PRIu32 ", \"calibration_data\": {", counter);
    // Print the calibration settings
    printf(
        "\"rx0_address\": \"%d\",\"rx1_address\":\"%d\",\"rx2_address\":\"%d\",\"bytes_to_read\":\"%d\", \"data\": [",
        calibration_settings->rx0_address,
        calibration_settings->rx1_address,
        calibration_settings->rx2_address,
        calibration_settings->bytes_to_read);
    for (uint32_t i = 0; i < (3 * (uint32_t)calibration_settings->bytes_to_read) / sizeof(uint32_t); i++)
    {
        if (i != 0)
        {
            printf(", ");
        }
        printf("\"%" PRIu32 "\"", calibration_data[i]);
    }
    printf("]}}\n");
}


void print_clustering_results(clustering_t* clustering, uint32_t counter)
{
    const uint16_t type = 3;  // clustering centroid type
    const uint32_t dummy = 0;

    printf("{\"ts\": %" PRIu32 ", \"detections\": {", counter);
    printf(
        "\"attributes\":[\"range_m\",\"azimuth_deg\",\"elevation_deg\",\"magnitude_dBFS\",\"speed_mps\",\"rx_idx\",\"range_bin\",\"doppler_bin\",\"sequence_num\",\"type\",\"seq_count_1\",\"seq_count_2\",\"ir_stat\"], \"data\": [");

    if (clustering->num_clusters == 0)
    {
        // if no centroids were detected, print an empty array
        printf("[%.2f, %.2f, %.2f, %.2f, %.2f, %d, %d, %d, %d, %d, %d, %d, %" PRIu32 "]",
               0.0f,
               0.0f,
               0.0f,
               0.0f,       // magnitude not available for clustering centroids
               0.0f,       // speed not available for clustering centroids
               0,       // rx antenna not available for clustering centroids
               0,       // range bin not available for clustering centroids
               0,       // doppler bin not available for clustering centroids
               0,       // sequence not available for clustering centroids
               type,
               0,       // seq_count_1 not available for clustering centroids
               0,       // seq_count_2 not available for clustering centroids
               dummy);  // ir_stat not available for clustering centroids
        printf("]}}\n");
    }
    else
    {
        // print detected centroids
        for (int32_t i = 0; i < clustering->num_clusters; i++)
        {
            if (i != 0)
            {
                printf(", ");
            }
            printf("[%.2f, %.2f, %.2f, %.2f, %.2f, %d, %d, %d, %d, %d, %d, %d, %" PRIu32 "]",
                   clustering->centroids[i].range_m,
                   clustering->centroids[i].azimuth_deg,
                   clustering->centroids[i].elevation_deg,
                   0.0f,      // magnitude not available for clustering centroids
                   0.0f,      // speed not available for clustering centroids
                   0,         // rx antenna not available for clustering centroids
                   0,         // range bin not available for clustering centroids
                   0,         // doppler bin not available for clustering centroids
                   0,         // sequence not available for clustering centroids
                   type,
                   0,         // seq_count_1 not available for clustering centroids
                   0,         // seq_count_2 not available for clustering centroids
                   dummy);    // ir_stat not available for clustering centroids
        }
        printf("]}}\n");
    }
}


void print_algo_processing_times(uint32_t counter, uint32_t motion_detection_elapsed_us,
                                 uint32_t clutter_localization_elapsed_us, uint32_t clustering_elapsed_us,
                                 uint32_t clock_speed_hz)
{
    static uint32_t min_motion_detection_time_us = UINT32_MAX;
    static uint32_t min_clutter_localization_time_us = UINT32_MAX;
    static uint32_t min_clustering_time_us = UINT32_MAX;

    static uint32_t max_motion_detection_time_us = 0;
    static uint32_t max_clutter_localization_time_us = 0;
    static uint32_t max_clustering_time_us = 0;

    static uint32_t avg_motion_detection_time_us = 0;
    static uint32_t avg_clutter_localization_time_us = 0;
    static uint32_t avg_clustering_time_us = 0;

    // update min times
    if (motion_detection_elapsed_us < min_motion_detection_time_us)
    {
        min_motion_detection_time_us = motion_detection_elapsed_us;
    }
    if (clutter_localization_elapsed_us < min_clutter_localization_time_us)
    {
        min_clutter_localization_time_us = clutter_localization_elapsed_us;
    }
    if (clustering_elapsed_us < min_clustering_time_us)
    {
        min_clustering_time_us = clustering_elapsed_us;
    }

    // update max times
    if (motion_detection_elapsed_us > max_motion_detection_time_us)
    {
        max_motion_detection_time_us = motion_detection_elapsed_us;
    }
    if (clutter_localization_elapsed_us > max_clutter_localization_time_us)
    {
        max_clutter_localization_time_us = clutter_localization_elapsed_us;
    }
    if (clustering_elapsed_us > max_clustering_time_us)
    {
        max_clustering_time_us = clustering_elapsed_us;
    }

    // update average times

    if (avg_motion_detection_time_us != 0)
    {
        avg_motion_detection_time_us += motion_detection_elapsed_us;
        avg_motion_detection_time_us /= 2;
    }
    else
    {
        avg_motion_detection_time_us = motion_detection_elapsed_us;
    }
    if (avg_clutter_localization_time_us != 0)
    {
        avg_clutter_localization_time_us += clutter_localization_elapsed_us;
        avg_clutter_localization_time_us /= 2;
    }
    else
    {
        avg_clutter_localization_time_us = clutter_localization_elapsed_us;
    }
    if (avg_clustering_time_us != 0)
    {
        avg_clustering_time_us += clustering_elapsed_us;
        avg_clustering_time_us /= 2;
    }
    else
    {
        avg_clustering_time_us = clustering_elapsed_us;
    }
    printf("{\"ts\": %" PRIu32 ", \"clock_speed_hz\":%" PRIu32 ", \"processing_times_us\": {", counter, clock_speed_hz);
    printf(
        "\"motion_detection_min\":%" PRIu32 ",\"motion_detection_max\":%" PRIu32 ",\"motion_detection_avg\":%" PRIu32 ",",
        min_motion_detection_time_us,
        max_motion_detection_time_us,
        avg_motion_detection_time_us);
    printf(
        "\"clutter_localization_min\":%" PRIu32 ",\"clutter_localization_max\":%" PRIu32 ",\"clutter_localization_avg\":%" PRIu32 ",",
        min_clutter_localization_time_us,
        max_clutter_localization_time_us,
        avg_clutter_localization_time_us);
    printf("\"clustering_min\":%" PRIu32 ",\"clustering_max\":%" PRIu32 ",\"clustering_avg\":%" PRIu32 "",
           min_clustering_time_us,
           max_clustering_time_us,
           avg_clustering_time_us);
    printf("}}\n");
}
