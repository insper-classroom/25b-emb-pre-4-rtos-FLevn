#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

enum { BTN_ID_R = 0, BTN_ID_Y = 1 };
static const uint BTN_PIN_R = 28;
static const uint BTN_PIN_Y = 21;
static const uint LED_PIN_R = 5;
static const uint LED_PIN_Y = 10;

static QueueHandle_t xQueueBtn = NULL;
static SemaphoreHandle_t xSemaphoreLedR;
static SemaphoreHandle_t xSemaphoreLedY;

void btn_callback(uint gpio, uint32_t events) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (events & GPIO_IRQ_EDGE_FALL) {
        uint8_t id;
        if (gpio == BTN_PIN_R)      id = BTN_ID_R;
        else if (gpio == BTN_PIN_Y) id = BTN_ID_Y;
        else return;
        xQueueSendFromISR(xQueueBtn, &id, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

static void btn_task(void *p) {
    gpio_init(BTN_PIN_R); gpio_set_dir(BTN_PIN_R, GPIO_IN); gpio_pull_up(BTN_PIN_R);
    gpio_init(BTN_PIN_Y); gpio_set_dir(BTN_PIN_Y, GPIO_IN); gpio_pull_up(BTN_PIN_Y);
    gpio_set_irq_enabled_with_callback(BTN_PIN_R, GPIO_IRQ_EDGE_FALL, true, &btn_callback);
    gpio_set_irq_enabled(BTN_PIN_Y, GPIO_IRQ_EDGE_FALL, true);

    uint8_t id;
    for(;;) {
        if (xQueueReceive(xQueueBtn, &id, portMAX_DELAY) == pdTRUE) {
            switch(id) {
                case BTN_ID_R:
                    xSemaphoreGive(xSemaphoreLedR);
                    break;
                case BTN_ID_Y:
                    xSemaphoreGive(xSemaphoreLedY);
                    break;
                default:
                    break;
            }
        }
    }
}

static void led_r_task(void *p) {
    gpio_init(LED_PIN_R); gpio_set_dir(LED_PIN_R, GPIO_OUT); gpio_put(LED_PIN_R, 0);
    bool blinking = false;
    for(;;) {
        if (xSemaphoreTake(xSemaphoreLedR, portMAX_DELAY) == pdTRUE) {
            blinking = !blinking;
            if (!blinking) {
                gpio_put(LED_PIN_R, 0);
                continue;
            }
            while (blinking) {
                gpio_put(LED_PIN_R, 1);
                if (xSemaphoreTake(xSemaphoreLedR, pdMS_TO_TICKS(100)) == pdTRUE) {
                    blinking = !blinking;
                    gpio_put(LED_PIN_R, 0);
                    break;
                }
                gpio_put(LED_PIN_R, 0);
                if (xSemaphoreTake(xSemaphoreLedR, pdMS_TO_TICKS(100)) == pdTRUE) {
                    blinking = !blinking;
                    gpio_put(LED_PIN_R, 0);
                    break;
                }
            }
        }
    }
}

static void led_y_task(void *p) {
    gpio_init(LED_PIN_Y); gpio_set_dir(LED_PIN_Y, GPIO_OUT); gpio_put(LED_PIN_Y, 0);
    bool blinking = false;
    for(;;) {
        if (xSemaphoreTake(xSemaphoreLedY, portMAX_DELAY) == pdTRUE) {
            blinking = !blinking;
            if (!blinking) {
                gpio_put(LED_PIN_Y, 0);
                continue;
            }
            while (blinking) {
                gpio_put(LED_PIN_Y, 1);
                if (xSemaphoreTake(xSemaphoreLedY, pdMS_TO_TICKS(100)) == pdTRUE) {
                    blinking = !blinking; gpio_put(LED_PIN_Y, 0); break;
                }
                gpio_put(LED_PIN_Y, 0);
                if (xSemaphoreTake(xSemaphoreLedY, pdMS_TO_TICKS(100)) == pdTRUE) {
                    blinking = !blinking; gpio_put(LED_PIN_Y, 0); break;
                }
            }
        }
    }
}

int main() {
    stdio_init_all();
    printf("Start RTOS exe5\n");

    xQueueBtn = xQueueCreate(8, sizeof(uint8_t));
    configASSERT(xQueueBtn);
    xSemaphoreLedR = xSemaphoreCreateBinary(); configASSERT(xSemaphoreLedR);
    xSemaphoreLedY = xSemaphoreCreateBinary(); configASSERT(xSemaphoreLedY);

    xTaskCreate(btn_task,   "btn",   256, NULL, 2, NULL);
    xTaskCreate(led_r_task, "led_r", 256, NULL, 1, NULL);
    xTaskCreate(led_y_task, "led_y", 256, NULL, 1, NULL);

    vTaskStartScheduler();
    while (1) {}
}
