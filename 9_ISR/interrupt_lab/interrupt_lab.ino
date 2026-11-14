#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

constexpr uint32_t TASK_DELAY_MS = 500; 

QueueHandle_t sequenceQueue = nullptr;
hw_timer_t *hwTimer = nullptr;

void ARDUINO_ISR_ATTR onTimer() {
    static uint32_t sequenceNumber = 1;
    
    // copies current number before queue just in case it is full, allowing for ISR to send next time it is called
    uint32_t valueToSend = sequenceNumber;
    sequenceNumber++;

    BaseType_t taskWoken = pdFALSE;
    
    xQueueSendFromISR(sequenceQueue, &valueToSend, &taskWoken);

    // Yield to the FreeRTOS schedulerif a higher priority task was woken
    if (taskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

// FreeRTOS Consumer Task
void consumerTask(void *pvParameters) {
    uint32_t receivedValue;

    while (true) {
        // Wait indefinitely for an item to appear in the queue
        if (xQueueReceive(sequenceQueue, &receivedValue, portMAX_DELAY) == pdTRUE) {
            
            // Print one sequence number per line
            Serial0.println(receivedValue);

            // Delay simulating task processing time (20 ms or 500 ms)
            vTaskDelay(pdMS_TO_TICKS(TASK_DELAY_MS));
        }
    }
}

void setup() {
    Serial0.begin(115200);

    sequenceQueue = xQueueCreate(4, sizeof(uint32_t));
    if (sequenceQueue == nullptr) {
        Serial0.println("Queue creation failed.");
        return;
    }

    if (xTaskCreate(consumerTask, "Consumer", 2048, nullptr, 1, nullptr) != pdPASS) {
        Serial0.println("Task creation failed.");
        return;
    }

    hwTimer = timerBegin(1000000); // init hardwareclock at 1MHz
    if (hwTimer == nullptr) {
        Serial0.println("Timer creation failed.");
        return;
    }

    timerAttachInterrupt(hwTimer, onTimer);
    timerAlarm(hwTimer, 100000, true, 0); 
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}