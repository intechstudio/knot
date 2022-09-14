/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/ringbuf.h"
#include "esp_log.h"
#include "esp_system.h"

#include "driver/gpio.h"


#include "freertos/queue.h"
#include "driver/uart.h"
#include "midi_translator.h"


#define EX_UART_NUM UART_NUM_1
#define PATTERN_CHR_NUM    (3)         /*!< Set the number of consecutive and identical characters received by receiver which defines a UART pattern*/

#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)
static QueueHandle_t uart0_queue;

static const char *TAG = "UART";

#include "driver/gpio.h"

#define TRS_TYPE_SELECT_PIN 16
#define SW_AB_PIN 35



RingbufHandle_t uart_rx_buffer_cvm;
RingbufHandle_t uart_rx_buffer_rtm;

uint8_t current_is_sysex = 0;

extern void led_tx_effect_start(void);
extern void led_rx_effect_start(void);
extern void led_err_effect_start(void);

void uart_init(){


    ESP_LOGI(TAG, "UART init");

    esp_log_level_set(TAG, ESP_LOG_INFO);

    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = 31250,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    //Install UART driver, and get the queue.
    uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart0_queue, 0);
    uart_param_config(EX_UART_NUM, &uart_config);

    //Set UART log level
    esp_log_level_set(TAG, ESP_LOG_INFO);
    //Set UART pins (using UART0 default pins ie no changes.)
    uart_set_pin(EX_UART_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_set_pin(EX_UART_NUM, 17, 18, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    //Set uart pattern detect function.
    uart_enable_pattern_det_baud_intr(EX_UART_NUM, '+', PATTERN_CHR_NUM, 9, 0, 0);
    //Reset the pattern queue length to record at most 20 pattern positions.
    uart_pattern_queue_reset(EX_UART_NUM, 20);


    gpio_set_direction(SW_AB_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(TRS_TYPE_SELECT_PIN, GPIO_MODE_OUTPUT);

    
    gpio_set_level(TRS_TYPE_SELECT_PIN, gpio_get_level(SW_AB_PIN));



    // Channel Voice Message Buffer
    #define UART_RX_BUFFER_CVM_SIZE 1600
    uart_rx_buffer_cvm = xRingbufferCreate(UART_RX_BUFFER_CVM_SIZE, RINGBUF_TYPE_NOSPLIT);
    if (uart_rx_buffer_cvm == NULL) {
        printf("Failed to create ring buffer\n");
    }

    // Real-Time Message Buffer
    #define UART_RX_BUFFER_RTM_SIZE 20
    uart_rx_buffer_rtm = xRingbufferCreate(UART_RX_BUFFER_RTM_SIZE, RINGBUF_TYPE_NOSPLIT);
    if (uart_rx_buffer_rtm == NULL) {
        printf("Failed to create ring buffer\n");
    }

}


int uart_send_data(struct uart_midi_event_packet ev)
{

    led_tx_effect_start();

    const int txBytes = uart_write_bytes(EX_UART_NUM, &ev.byte1, ev.length);
    //ESP_LOGI(logName, "Wrote %d bytes %d %d %d", txBytes, data[0], data[1], data[2]);
    return txBytes;
}


void uart_rx_decode_task(void *arg){



    SemaphoreHandle_t signaling_sem = (SemaphoreHandle_t)arg;
    //xSemaphoreTake(signaling_sem, portMAX_DELAY);

    ESP_LOGI(TAG, "UART TX init done");

    for(;;) {
        
        for(uint8_t i = 0; i<3; i++) {

            gpio_set_level(TRS_TYPE_SELECT_PIN, gpio_get_level(SW_AB_PIN));
            //ESP_LOGI(TAG, "UART TX loop");
            //uart_write_bytes(EX_UART_NUM, "UUUU", 4);

            uint8_t* item_0_ptr = 0;
            uint8_t* item_1_ptr = 0;
            uint8_t* item_2_ptr = 0;

            uint16_t item_0 = -1;
            uint16_t item_1 = -1;
            uint16_t item_2 = -1;


            item_0_ptr = xRingbufferReceive(uart_rx_buffer_cvm, NULL, 0);
            if (item_0_ptr !=  NULL){
                item_0 = *item_0_ptr;
                vRingbufferReturnItem(uart_rx_buffer_cvm, (void*)item_0_ptr);

                if (current_is_sysex){

                    if (item_0 == 0xF7){
                        // sysex ended with sigle byte
                        struct usb_midi_event_packet ep = {.byte0 = 0x05, .byte1 = 0xF7, .byte2 = 0x00, .byte3 = 0x00};
                        ESP_LOGI(TAG, "To USB: SYSEX: %d %d %d %d", ep.byte0, ep.byte1, ep.byte2, ep.byte3);
                        current_is_sysex = 0;
                    }
                    else if (item_0 > 127){
                        ESP_LOGI(TAG, "To USB: Invalid sysex error");
                    }
                    else{

                        item_1_ptr = xRingbufferReceive(uart_rx_buffer_cvm, NULL, 0);

                        if (item_1_ptr !=  NULL){
                            item_1 = *item_1_ptr;
                            vRingbufferReturnItem(uart_rx_buffer_cvm, (void*)item_1_ptr);
                        
                            if (item_1 == 0xF7){
                                // sysex ended with two bytes
                                struct usb_midi_event_packet ep = {.byte0 = 0x06, .byte1 = (uint8_t)item_0, .byte2 = 0xF7, .byte3 = 0x00};
                                ESP_LOGI(TAG, "To USB: SYSEX: %d %d %d %d", ep.byte0, ep.byte1, ep.byte2, ep.byte3);
                                current_is_sysex = 0;
                            }
                            else if (item_1 > 127){
                                ESP_LOGI(TAG, "To USB: Invalid sysex error");
                            }
                            else{


                                item_2_ptr = xRingbufferReceive(uart_rx_buffer_cvm, NULL, 0);

                                if (item_2_ptr !=  NULL){
                                    item_2 = *item_2_ptr;
                                    vRingbufferReturnItem(uart_rx_buffer_cvm, (void*)item_2_ptr);
                                
                                    if (item_2 == 0xF7){
                                        // sysex ended with three bytes
                                        struct usb_midi_event_packet ep = {.byte0 = 0x07, .byte1 = (uint8_t)item_0, .byte2 = (uint8_t)item_1, .byte3 = 0xF7};
                                        ESP_LOGI(TAG, "To USB: SYSEX: %d %d %d %d", ep.byte0, ep.byte1, ep.byte2, ep.byte3);
                                        current_is_sysex = 0;
                                    }
                                    else if (item_2 > 127){
                                        ESP_LOGI(TAG, "To USB: Invalid sysex error");
                                    }
                                    else{
                                        // sysex continues with three bytes
                                        struct usb_midi_event_packet ep = {.byte0 = 0x04, .byte1 = (uint8_t)item_0, .byte2 = (uint8_t)item_1, .byte3 = (uint8_t)item_2};
                                        ESP_LOGI(TAG, "To USB: SYSEX: %d %d %d %d", ep.byte0, ep.byte1, ep.byte2, ep.byte3);
                                        
                                    }

                                }


                                
                            }

                        }

                



                    }

                }
                else if (item_0 == 0xF0){ // sysex start



                    current_is_sysex = 1;

                    item_1_ptr = xRingbufferReceive(uart_rx_buffer_cvm, NULL, 0);
        

                    if (item_1_ptr !=  NULL){
                        item_1 = *item_1_ptr;
                        vRingbufferReturnItem(uart_rx_buffer_cvm, (void*)item_1_ptr);
                    
                        if (item_1 == 0xF7){
                            // sysex ended with two bytes
                            struct usb_midi_event_packet ep = {.byte0 = 0x06, .byte1 = (uint8_t)item_0, .byte2 = 0xF7, .byte3 = 0x00};
                            ESP_LOGI(TAG, "To USB: SYSEX: %d %d %d %d", ep.byte0, ep.byte1, ep.byte2, ep.byte3);
                            current_is_sysex = 0;
                        }
                        else if (item_1 > 127){
                            ESP_LOGI(TAG, "To USB: Invalid sysex error");
                        }
                        else{


                            item_2_ptr = xRingbufferReceive(uart_rx_buffer_cvm, NULL, 0);

                            if (item_2_ptr !=  NULL){
                                item_2 = *item_2_ptr;
                                vRingbufferReturnItem(uart_rx_buffer_cvm, (void*)item_2_ptr);
                            
                                if (item_2 == 0xF7){
                                    // sysex ended with three bytes
                                    struct usb_midi_event_packet ep = {.byte0 = 0x07, .byte1 = (uint8_t)item_0, .byte2 = (uint8_t)item_1, .byte3 = 0xF7};
                                    ESP_LOGI(TAG, "To USB: SYSEX: %d %d %d %d", ep.byte0, ep.byte1, ep.byte2, ep.byte3);
                                    current_is_sysex = 0;
                                }
                                else if (item_2 > 127){
                                    ESP_LOGI(TAG, "To USB: Invalid sysex error");
                                }
                                else{
                                    // sysex continues with three bytes
                                    struct usb_midi_event_packet ep = {.byte0 = 0x04, .byte1 = (uint8_t)item_0, .byte2 = (uint8_t)item_1, .byte3 = (uint8_t)item_2};
                                    ESP_LOGI(TAG, "To USB: SYSEX: %d %d %d %d", ep.byte0, ep.byte1, ep.byte2, ep.byte3);
                                    
                                }

                            }


                            
                        }

                    }

                


                }
                else if(item_0 > 127){ // channel voice message


                    item_1_ptr = xRingbufferReceive(uart_rx_buffer_cvm, NULL, 0);
                    if (item_1_ptr !=  NULL){
                        item_1 = *item_1_ptr;
                        vRingbufferReturnItem(uart_rx_buffer_cvm, (void*)item_1_ptr);
                    }

                    item_2_ptr = xRingbufferReceive(uart_rx_buffer_cvm, NULL, 0);
                    if (item_2_ptr !=  NULL){

                        item_2 = *item_2_ptr;
                        vRingbufferReturnItem(uart_rx_buffer_cvm, (void*)item_2_ptr);
                    }

                    if (item_0 != (uint16_t)-1 || item_1 != (uint16_t)-1 || item_2 != (uint16_t)-1){
                        //ESP_LOGI(TAG, "To USB: CVM: %d %d %d", item_0, item_1, item_2);
                    }

                }
                else{
                    ESP_LOGI(TAG, "To USB: Invalid Packet %d", item_0);
                }


            }

        }

        vTaskDelay(pdMS_TO_TICKS(10));


    }
}


void uart_rx_task(void *arg)
{


    SemaphoreHandle_t signaling_sem = (SemaphoreHandle_t)arg;
    //xSemaphoreTake(signaling_sem, portMAX_DELAY);

    uart_event_t event;
    size_t buffered_size;
    uint8_t* dtmp = (uint8_t*) malloc(RD_BUF_SIZE);


    ESP_LOGI(TAG, "UART RX init done");

    for(;;) {


        //Waiting for UART event.
        if(xQueueReceive(uart0_queue, (void * )&event, (TickType_t)portMAX_DELAY)) {
            bzero(dtmp, RD_BUF_SIZE);
            //ESP_LOGI(TAG, "uart[%d] event:", EX_UART_NUM);
            switch(event.type) {
                //Event of UART receving data
                /*We'd better handler data event fast, there would be much more data events than
                other types of events. If we take too much time on data event, the queue might
                be full.*/
                case UART_DATA:
                    
                    led_rx_effect_start();
                    
                    //ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
                    uart_read_bytes(EX_UART_NUM, dtmp, event.size, portMAX_DELAY);
                    
                    for(uint8_t i = 0; i<event.size; i++){

                        //Add received bytes to ringbuffer based on message type
                        if (uart_midi_is_byte_rtm(dtmp[i])){

                            UBaseType_t res =  xRingbufferSend(uart_rx_buffer_rtm, &dtmp[i], sizeof(dtmp[i]), 0);
                            if (res != pdTRUE) {
                                printf("RTM: FAIL\n");
                                led_err_effect_start();
                            }
                        }
                        else{

                            UBaseType_t res =  xRingbufferSend(uart_rx_buffer_cvm, &dtmp[i], sizeof(dtmp[i]), 0);
                            if (res != pdTRUE) {
                                printf("CVM: FAIL\n");
                                led_err_effect_start();
                            }
                        }

                    }
                    
                    //ESP_LOGI(TAG, "[DATA EVT]: %d %d %d %d", dtmp[0], dtmp[1], dtmp[2], dtmp[3]);
                    break;
                //Event of HW FIFO overflow detected
                case UART_FIFO_OVF:
                    ESP_LOGI(TAG, "hw fifo overflow");
                    // If fifo overflow happened, you should consider adding flow control for your application.
                    // The ISR has already reset the rx FIFO,
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(EX_UART_NUM);
                    xQueueReset(uart0_queue);
                    break;
                //Event of UART ring buffer full
                case UART_BUFFER_FULL:
                    ESP_LOGI(TAG, "ring buffer full");
                    // If buffer full happened, you should consider encreasing your buffer size
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(EX_UART_NUM);
                    xQueueReset(uart0_queue);
                    break;
                //Event of UART RX break detected
                case UART_BREAK:
                    ESP_LOGI(TAG, "uart rx break");
                    break;
                //Event of UART parity check error
                case UART_PARITY_ERR:
                    ESP_LOGI(TAG, "uart parity error");
                    break;
                //Event of UART frame error
                case UART_FRAME_ERR:
                    ESP_LOGI(TAG, "uart frame error");
                    break;
                //UART_PATTERN_DET
                case UART_PATTERN_DET:
                    uart_get_buffered_data_len(EX_UART_NUM, &buffered_size);
                    int pos = uart_pattern_pop_pos(EX_UART_NUM);
                    ESP_LOGI(TAG, "[UART PATTERN DETECTED] pos: %d, buffered size: %d", pos, buffered_size);
                    if (pos == -1) {
                        // There used to be a UART_PATTERN_DET event, but the pattern position queue is full so that it can not
                        // record the position. We should set a larger queue size.
                        // As an example, we directly flush the rx buffer here.
                        uart_flush_input(EX_UART_NUM);
                    } else {
                        uart_read_bytes(EX_UART_NUM, dtmp, pos, 100 / portTICK_PERIOD_MS);
                        uint8_t pat[PATTERN_CHR_NUM + 1];
                        memset(pat, 0, sizeof(pat));
                        uart_read_bytes(EX_UART_NUM, pat, PATTERN_CHR_NUM, 100 / portTICK_PERIOD_MS);
                        ESP_LOGI(TAG, "read data: %s", dtmp);
                        ESP_LOGI(TAG, "read pat : %s", pat);
                    }
                    break;
                //Others
                default:
                    ESP_LOGI(TAG, "uart event type: %d", event.type);
                    break;
            }
        }

        //ESP_LOGI(TAG, "UART RX loop");
    }
    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);

}
