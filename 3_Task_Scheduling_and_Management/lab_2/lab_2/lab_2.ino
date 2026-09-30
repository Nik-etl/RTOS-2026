#define RGB_BUILTIN 38
#define RGB_BRIGHTNESS 50

// Shared Variable
volatile uint32_t blinkInterval = 500;

// Task Handles
TaskHandle_t serialTaskHandle = NULL;
TaskHandle_t ledTaskHandle = NULL;

void serialTask(void *parameter) {
  for (;;) {
    if (Serial.available() > 0) {
      // Read the incoming string until a newline character
      String input = Serial.readStringUntil('\n');
      input.trim(); // Remove any carriage returns or extra spaces

      if (input == "250" || input == "500" || input == "1000") {
        blinkInterval = input.toInt();
        Serial.print("Interval updated to: ");
        Serial.println(blinkInterval);
      } 
      else if (input == "suspend") {
        Serial.println("Suspending LED Task...");
        vTaskSuspend(ledTaskHandle);
        Serial.println("LED Task Suspended.");
      } 
      else if (input == "resume") {
        Serial.println("Resuming LED Task...");
        vTaskResume(ledTaskHandle);
        Serial.println("LED Task Resumed.");
      } 
      else if (input.length() > 0) {
        Serial.println("Invalid input. Use: 250, 500, 1000, suspend, or resume.");
      }
    }
    // Yield to scheduler while waiting for input
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void ledTask(void *parameter) {
  for (;;) {
    // LED ON
    neopixelWrite(RGB_BUILTIN, RGB_BRIGHTNESS, 50, 0); // Red (or change to your preference)
    vTaskDelay(pdMS_TO_TICKS(blinkInterval));
    
    // LED OFF
    neopixelWrite(RGB_BUILTIN, 0, 0, 0);
    vTaskDelay(pdMS_TO_TICKS(blinkInterval));
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000); // Allow Serial Monitor to connect

  Serial.println("FreeRTOS Task Control Started.");
  Serial.println("Commands: 250, 500, 1000, suspend, resume");

  // Create Serial Task
  xTaskCreatePinnedToCore(
    serialTask,
    "Serial Task",
    4096,             // String operations require a bit more stack space
    NULL,
    1,                // Priority 1
    &serialTaskHandle,
    1                 // Core 1
  );

  // Create LED Task
  xTaskCreatePinnedToCore(
    ledTask,
    "LED Task",
    2048,
    NULL,
    1,                // Priority 1
    &ledTaskHandle,
    1                 // Core 1
  );
}

void loop() {
  // FreeRTOS scheduler takes over
  vTaskDelay(portMAX_DELAY); 
}