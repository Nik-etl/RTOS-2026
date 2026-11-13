#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

constexpr bool USE_COUNTING = false;  // Set to true for the second run.
constexpr int LED_PIN = 4;
constexpr unsigned PACKAGE_COUNT = 5;

SemaphoreHandle_t packageSem = nullptr;
TaskHandle_t packageTaskHandle = nullptr;
hw_timer_t *packageTimer = nullptr;

void ARDUINO_ISR_ATTR onPackage() {
    static unsigned generated = 0;

    if (generated == PACKAGE_COUNT) {
        return;
    }

    // Each interrupt represents one package passing the sensor.
    xSemaphoreGiveFromISR(packageSem, nullptr);
    ++generated;

    // Tell the task to start counting only after the fifth package.
    if (generated == PACKAGE_COUNT) {
        BaseType_t taskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(packageTaskHandle, &taskWoken);

        if (taskWoken == pdTRUE) {
            portYIELD_FROM_ISR();
        }
    }
}

void packageTask(void *) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    timerStop(packageTimer);

    Serial0.println("Sensor generated 5 package signals.");

    unsigned counted = 0;
    while (xSemaphoreTake(packageSem, 0) == pdTRUE) {
        ++counted;
        Serial0.printf("LED flash %u\n", counted);

        digitalWrite(LED_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(300));
        digitalWrite(LED_PIN, LOW);
        vTaskDelay(pdMS_TO_TICKS(300));
    }

    Serial0.printf("Packages counted: %u\n", counted);
    vTaskDelete(nullptr);
}

void setup() {
    Serial0.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    if (USE_COUNTING) {
        packageSem = xSemaphoreCreateCounting(PACKAGE_COUNT, 0);
    } else {
        packageSem = xSemaphoreCreateBinary();
    }

    if (packageSem == nullptr) {
        Serial0.println("Semaphore creation failed.");
        return;
    }

    packageTimer = timerBegin(1000000);
    if (packageTimer == nullptr) {
        Serial0.println("Timer creation failed.");
        return;
    }

    if (xTaskCreate(packageTask, "Packages", 4096, nullptr, 2,
                    &packageTaskHandle) != pdPASS) {
        Serial0.println("Task creation failed.");
        timerEnd(packageTimer);
        return;
    }

    Serial0.println(USE_COUNTING ? "Counting semaphore" : "Binary semaphore");
    timerAttachInterrupt(packageTimer, onPackage);
    timerAlarm(packageTimer, 100000, true, 0);  // One interrupt every 100 ms.
}

void loop() {
    delay(1000);
}