/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "knot_platform.h"
#include "esp_timer.h"

#include "esp_heap_caps.h"

#include "rom/ets_sys.h" // For ets_printf

void* grid_platform_allocate_volatile(size_t size) {

  void* handle = heap_caps_malloc(size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

  // ets_printf("ADDRESS: %lx\r\n", handle);

  if (handle == NULL) {

    ets_printf("MALLOC FAILED");

    while (1) {
    }
  }

  return handle;
}

#include "esp_random.h"
uint8_t grid_platform_get_random_8() {
  uint32_t random_number = esp_random();
  return random_number % 256;
}

void grid_platform_delay_ms(uint32_t delay_milliseconds) { ets_delay_us(delay_milliseconds * 1000); }

uint8_t grid_platform_get_adc_bit_depth(void) { return 12; }

uint64_t IRAM_ATTR grid_platform_rtc_get_micros(void) { return esp_timer_get_time(); }

uint64_t IRAM_ATTR grid_platform_rtc_get_elapsed_time(uint64_t told) { return grid_platform_rtc_get_micros() - told; }

uint64_t IRAM_ATTR grid_platform_rtc_get_diff(uint64_t t1, uint64_t t2) { return ((t1 << 1) - (t2 << 1)) >> 1; }
