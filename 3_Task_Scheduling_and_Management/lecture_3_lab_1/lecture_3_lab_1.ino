TaskHandle_t TaskA_Handle;
TaskHandle_t TaskB_Handle;

void TaskA(void *pvParameters) {
  const char *message = "Task A is printing slowly...";

  for (;;) {
    for (int i = 0; i < strlen(message); i++) {
      Serial.print(message[i]);

      // Notify Task B roughly halfway through the message
      if (i == strlen(message)/2) {
        xTaskNotifyGive(TaskB_Handle);
      }

      // vTaskDelay is used here purely to slow down the loop so you can read it.
      // It does not hide the scheduler preemption, which happens instantly on the line above.
      // vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    
    Serial.println();
    vTaskDelay(2000 / portTICK_PERIOD_MS); // Pause before repeating the loop
  }
}

void TaskB(void *pvParameters) {
  for (;;) {
    // pdTRUE clears the notification value back to 0.
    // portMAX_DELAY tells the task to block forever until a notification arrives.
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    // Because this task is Priority 2, it will print this EXACTLY when notified, 
    // interrupting Task A's message mid-sentence.
    Serial.print("\n[Task B: Notification Received!]\n");
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000); // Brief pause to allow the Serial Monitor to connect

  // IMPORTANT: Create Task B first so 'TaskB_Handle' is initialized 
  // before Task A starts running and tries to notify it.
  xTaskCreatePinnedToCore(
    TaskB,
    "Task_B",
    2048,
    NULL,
    1,              // Priority 2 (Higher)
    &TaskB_Handle,  // Store the handle for Task A to use
    1
  );

  xTaskCreatePinnedToCore(
    TaskA,
    "Task_A",
    2048,
    NULL,
    1,              // Priority 1 (Lower)
    NULL,           // No handle needed for Task A
    1
  );
}

void loop() {
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}