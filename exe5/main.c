#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include <stdio.h>
#include <string.h> 
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/irq.h"

const int BTN_PIN_R = 28;
const int BTN_PIN_Y = 21;

const int LED_PIN_R = 5;
const int LED_PIN_Y = 10;

QueueHandle_t xQueueButId;
SemaphoreHandle_t xSemaphore_r;
SemaphoreHandle_t xSemaphore_y;

typedef struct {
    int led_id; // 0 for RED, 1 for YELLOW
    int delay;
} LedMsg_t;

void btn_callback(uint gpio, uint32_t events) {
    if (events == 0x4) { // fall edge
        if (gpio == BTN_PIN_R)
            xSemaphoreGiveFromISR(xSemaphore_r, 0);
        else if (gpio == BTN_PIN_Y)
            xSemaphoreGiveFromISR(xSemaphore_y, 0);
    }
}

void led_1_task(void *p) {
    gpio_init(LED_PIN_R);
    gpio_set_dir(LED_PIN_R, GPIO_OUT);

    LedMsg_t msg;
    while (true) {
        if (xQueueReceive(xQueueButId, &msg, portMAX_DELAY)) {
            if (msg.led_id == 0) {
                printf("LED R delay: %d\n", msg.delay);
                if (msg.delay > 0) {
                    gpio_put(LED_PIN_R, 1);
                    vTaskDelay(pdMS_TO_TICKS(msg.delay));
                    gpio_put(LED_PIN_R, 0);
                    vTaskDelay(pdMS_TO_TICKS(msg.delay));
                } else {
                    gpio_put(LED_PIN_R, 0);
                }
            }
        }
    }
}

void led_2_task(void *p) {
    gpio_init(LED_PIN_Y);
    gpio_set_dir(LED_PIN_Y, GPIO_OUT);

    LedMsg_t msg;
    while (true) {
        if (xQueueReceive(xQueueButId, &msg, portMAX_DELAY)) {
            if (msg.led_id == 1) {
                printf("LED Y delay: %d\n", msg.delay);
                if (msg.delay > 0) {
                    gpio_put(LED_PIN_Y, 1);
                    vTaskDelay(pdMS_TO_TICKS(msg.delay));
                    gpio_put(LED_PIN_Y, 0);
                    vTaskDelay(pdMS_TO_TICKS(msg.delay));
                } else {
                    gpio_put(LED_PIN_Y, 0);
                }
            }
        }
    }
}

void btn_1_task(void *p) {
    gpio_init(BTN_PIN_R);
    gpio_set_dir(BTN_PIN_R, GPIO_IN);
    gpio_pull_up(BTN_PIN_R);
    gpio_set_irq_enabled_with_callback(BTN_PIN_R, GPIO_IRQ_EDGE_FALL, true,
                                       &btn_callback);

    int delay = 0;
    LedMsg_t msg;
    msg.led_id = 0;
    while (true) {
        if (xSemaphoreTake(xSemaphore_r, pdMS_TO_TICKS(500)) == pdTRUE) {
            delay = (delay == 100) ? 0 : 100;
            printf("delay btn R %d \n", delay);
            msg.delay = delay;
            xQueueSend(xQueueButId, &msg, 0);
        }
    }
}

void btn_2_task(void *p) {
    gpio_init(BTN_PIN_Y);
    gpio_set_dir(BTN_PIN_Y, GPIO_IN);
    gpio_pull_up(BTN_PIN_Y);
    gpio_set_irq_enabled(BTN_PIN_Y, GPIO_IRQ_EDGE_FALL, true);

    int delay = 0;
    LedMsg_t msg;
    msg.led_id = 1;
    while (true) {
        if (xSemaphoreTake(xSemaphore_y, pdMS_TO_TICKS(500)) == pdTRUE) {
            delay = (delay == 100) ? 0 : 100;
            printf("delay btn Y %d \n", delay);
            msg.delay = delay;
            xQueueSend(xQueueButId, &msg, 0);
        }
    }
}

int main() {
    stdio_init_all();
    printf("Start RTOS \n");

    xQueueButId = xQueueCreate(32, sizeof(LedMsg_t));
    xSemaphore_r = xSemaphoreCreateBinary();
    xSemaphore_y = xSemaphoreCreateBinary();

    xTaskCreate(led_1_task, "LED_Task 1", 256, NULL, 1, NULL);
    xTaskCreate(btn_1_task, "BTN_Task 1", 256, NULL, 1, NULL);
    xTaskCreate(led_2_task, "LED_Task 2", 256, NULL, 1, NULL);
    xTaskCreate(btn_2_task, "BTN_Task 2", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}
