/*
  Lecture 12: Multicore Systems
  ESP32-S3 Dual-Core Producer-Consumer-Monitor Demonstration
  Board: ESP32-S3 DevKitC-1 | Serial Monitor: 115200 baud
  Architecture: Producer (Core 0) -> Queue -> Consumer (Core 1) -> Shared Statistics <-> Monitor (loop())
*/
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

struct SensorReading {
  uint32_t sequence;
  float value;
  TickType_t producedAt;
};

struct SharedStatistics {
  uint32_t sampleCount;
  float value;
  float sum;
  float runningAverage;
  float minimum;
  float maximum;
};

constexpr UBaseType_t QUEUE_LENGTH = 8;
constexpr uint32_t PRODUCER_PERIOD_MS = 500;
constexpr uint32_t QUEUE_WAIT_MS = 100;
constexpr uint32_t MUTEX_WAIT_MS = 50;
constexpr uint32_t MONITOR_PERIOD_MS = 3000;
constexpr uint32_t PRODUCER_STACK = 4096;
constexpr uint32_t CONSUMER_STACK = 4096;
constexpr uint32_t ALARM_STACK = 4096;

constexpr uint32_t THRESHOLD = 24f;


QueueHandle_t readingQueue = nullptr;
SemaphoreHandle_t statisticsMutex = nullptr, alarmSem;
SharedStatistics statistics = {0, 0.0f, 0.0f, 0.0f, 0.0f};

void fatalError(const char *message) {
  Serial.printf("FATAL: %s\n", message);
  Serial.flush();
  while (true) {
    delay(1000);
  }
}

float makeSimulatedReading(uint32_t sequence) {
  // Temperature-like signal: 22.0, 22.4, 22.8 ... 24.8, then repeat
  return 22.0f + static_cast<float>(sequence % 8U) * 0.4f;
}

// TASK 1: SENSOR PRODUCER (Core 0)
void sensorProducerTask(void *parameter) {
  (void)parameter;
  uint32_t sequence = 0;
  TickType_t lastWake = xTaskGetTickCount();
  for (;;) {
    SensorReading reading;
    reading.sequence = sequence++;
    reading.value = makeSimulatedReading(reading.sequence);
    reading.producedAt = xTaskGetTickCount();

    if (xQueueSend(readingQueue, &reading, pdMS_TO_TICKS(QUEUE_WAIT_MS)) == pdPASS) {
      Serial.printf("[Producer | Core %d] seq=%lu value=%.2f queued=%u\n",
                    xPortGetCoreID(),
                    static_cast<unsigned long>(reading.sequence),
                    reading.value,
                    static_cast<unsigned>(uxQueueMessagesWaiting(readingQueue)));
    } else {
      Serial.printf("[Producer | Core %d] queue full, dropped seq=%lu\n",
                    xPortGetCoreID(),
                    static_cast<unsigned long>(reading.sequence));
    }
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(PRODUCER_PERIOD_MS));
  }
}

// TASK 2: DATA CONSUMER (Core 1)
void dataConsumerTask(void *parameter) {
  (void)parameter;
  SensorReading reading;
  for (;;) {
    if (xQueueReceive(readingQueue, &reading, portMAX_DELAY) == pdPASS) {
      SharedStatistics snapshot;
      bool updated = false;
      if(reading.value > THRESHOLD) xSemaphoreGive(alarmSem); // trigger alarm

      if (xSemaphoreTake(statisticsMutex, pdMS_TO_TICKS(MUTEX_WAIT_MS)) == pdTRUE) {
        if (statistics.sampleCount == 0) {
          statistics.minimum = reading.value;
          statistics.maximum = reading.value;
        } else {
          if (reading.value < statistics.minimum) statistics.minimum = reading.value;
          if (reading.value > statistics.maximum) statistics.maximum = reading.value;
        }
        statistics.sampleCount++;
        statistics.sum += reading.value;
        statistics.runningAverage = statistics.sum / statistics.sampleCount;
        snapshot = statistics;
        updated = true;
        xSemaphoreGive(statisticsMutex);
      }

      if (updated) {
        Serial.printf("[Consumer | Core %d] seq=%lu avg=%.2f min=%.2f max=%.2f count=%lu value=%.2f \n",
                      xPortGetCoreID(),
                      static_cast<unsigned long>(reading.sequence),
                      snapshot.runningAverage,
                      snapshot.minimum,
                      snapshot.maximum,
                      static_cast<unsigned long>(snapshot.sampleCount),
                      snapshot.value);
      } else {
        Serial.printf("[Consumer | Core %d] mutex timeout for seq=%lu\n",
                      xPortGetCoreID(),
                      static_cast<unsigned long>(reading.sequence));
      }
    }
  }
}

// TASK 2: DATA CONSUMER (Core 1) Interrupt would be more useful and faster in reality this is just for the assignment
void alarmTask(void *parameter){
  (void)parameter;
  for(;;){
    xSemaphoreTake(alarmSem, portMAX_DELAY);
    Serial.printf("[ALARM] Temperature too high\n");
  }
}

// SETUP
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("Lecture 12 - ESP32-S3 Dual-Core Queue + Mutex Demo");
  Serial.printf("setup() is running on core %d\n", xPortGetCoreID());

  readingQueue = xQueueCreate(QUEUE_LENGTH, sizeof(SensorReading));
  if (readingQueue == nullptr) {
    fatalError("Could not create reading queue");
  }

  statisticsMutex = xSemaphoreCreateMutex();
  if (statisticsMutex == nullptr) {
    vQueueDelete(readingQueue);
    readingQueue = nullptr;
    fatalError("Could not create statistics mutex");
  }

  alarmSem = xSemaphoreCreateBinary();
  if (alarmSem == nullptr) {
    fatalError("Could not create reading queue");
  }

  BaseType_t producerCreated = xTaskCreatePinnedToCore(
      sensorProducerTask, "SensorProducer", PRODUCER_STACK, nullptr, 2, nullptr, 0);
  if (producerCreated != pdPASS) {
    fatalError("Could not create producer task on Core 0");
  }

  BaseType_t consumerCreated = xTaskCreatePinnedToCore(
      dataConsumerTask, "DataConsumer", CONSUMER_STACK, nullptr, 2, nullptr, 1);
  if (consumerCreated != pdPASS) {
    fatalError("Could not create consumer task on Core 1");
  }

    BaseType_t alarmCreated = xTaskCreatePinnedToCore(
      alarmTask, "Alarm", ALARM_STACK, nullptr, 3, nullptr, 1);
  if (alarmCreated != pdPASS) {
    fatalError("Could not create alarm task on Core 1");
  }

  Serial.println("Tasks created. Watch the core IDs below.");
}

// MONITOR (Arduino loop())
void loop() {
  static uint32_t lastMonitorTime = 0;
  if (millis() - lastMonitorTime >= MONITOR_PERIOD_MS) {
    lastMonitorTime = millis();
    SharedStatistics snapshot;
    bool copied = false;

    if (xSemaphoreTake(statisticsMutex, pdMS_TO_TICKS(MUTEX_WAIT_MS)) == pdTRUE) {
      snapshot = statistics;
      copied = true;
      xSemaphoreGive(statisticsMutex);
    }

    if (copied) {
      Serial.printf("[Monitor  | Core %d] count=%lu avg=%.2f min=%.2f max=%.2f\n",
                    xPortGetCoreID(),
                    static_cast<unsigned long>(snapshot.sampleCount),
                    snapshot.runningAverage,
                    snapshot.minimum,
                    snapshot.maximum);
    } else {
      Serial.printf("[Monitor  | Core %d] mutex timeout\n", xPortGetCoreID());
    }
  }
  delay(10);
}