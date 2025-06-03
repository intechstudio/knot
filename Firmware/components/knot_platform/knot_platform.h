/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#pragma once

#include "esp_attr.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void* grid_platform_allocate_volatile(size_t size);
uint8_t grid_platform_get_random_8(void);

void grid_platform_delay_ms(uint32_t delay_milliseconds);

uint8_t grid_platform_get_adc_bit_depth(void);


#ifdef __cplusplus
}
#endif
