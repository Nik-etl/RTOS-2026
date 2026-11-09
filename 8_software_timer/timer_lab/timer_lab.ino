#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include <U8g2lib.h>
#include <Wire.h>

// --- Hardware Pins ---
constexpr uint8_t SDA_PIN = 8;
constexpr uint8_t SCL_PIN = 9;
constexpr uint8_t LED_PIN = 6;

// --- OLED Display Object ---
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(/* rotation=*/U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// --- Task Notification Event Bits ---
constexpr uint32_t EVENT_TICK     = (1 << 0); //1 shifted 0 (01)
constexpr uint32_t EVENT_COMPLETE = (1 << 1); //1 shifted 1 (10)

// --- Global Variables ---
TaskHandle_t mainTaskHandle = NULL;
TimerHandle_t periodicTimer;
TimerHandle_t oneShotTimer;
TimerHandle_t resetTimer;
constexpr uint32_t EVENT_RESET    = (1 << 2); // event for returning to ready

enum SessionState { //state machine
  STATE_READY,
  STATE_RUNNING,
  STATE_COMPLETE
};

SessionState currentState = STATE_READY;
int countdown_value = 10;

// --- Timer Callbacks ---
// 1000ms Auto-reload Timer Callback
void periodicTimerCallback(TimerHandle_t xTimer) {
  // Send tick event to the main application task
  if (mainTaskHandle != NULL) {
    xTaskNotify(mainTaskHandle, EVENT_TICK, eSetBits);
  }
}

// 10,000ms One-shot Timer Callback
void oneShotTimerCallback(TimerHandle_t xTimer) {
  // Send completion event to the main application task
  if (mainTaskHandle != NULL) {
    xTaskNotify(mainTaskHandle, EVENT_COMPLETE, eSetBits);
  }
}

void resetTimerCallback(TimerHandle_t xTimer) {
  if (mainTaskHandle != NULL) {
    xTaskNotify(mainTaskHandle, EVENT_RESET, eSetBits);
  }
}

// --- OLED Drawing Helpers ---
void drawReady() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB10_tr);
  u8g2.drawStr(10, 20, "Press S");
  u8g2.drawStr(10, 40, "");
  u8g2.sendBuffer();
}

void drawRunning() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB10_tr);
  u8g2.drawStr(10, 20, "Remaining:");
  u8g2.setCursor(10, 45);
  u8g2.print(countdown_value);
  u8g2.print(" s");
  u8g2.sendBuffer();
}

void drawComplete() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB10_tr);
  u8g2.drawStr(10, 35, "Session complete");
  u8g2.sendBuffer();
}

void setup() {
  Serial.begin(115200);
  
  // Capture the handle of the main Arduino loop() task for notifications
  mainTaskHandle = xTaskGetCurrentTaskHandle();

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Wire.begin(SDA_PIN, SCL_PIN);
  u8g2.begin();

  // 1. Create Periodic Timer (1000 ms)
  periodicTimer = xTimerCreate(
    "TickTimer",
    pdMS_TO_TICKS(1000),
    pdTRUE,               // Auto-reload
    (void *)0,
    periodicTimerCallback
  );

  // 2. Create One-Shot Timer (10,000 ms)
  oneShotTimer = xTimerCreate(
    "CompleteTimer",
    pdMS_TO_TICKS(10000),
    pdFALSE,              // One-shot
    (void *)1,
    oneShotTimerCallback
  );
  // 3. Create event reset timer
  resetTimer = xTimerCreate(
    "ResetTimer",
    pdMS_TO_TICKS(1000),  // 1-second delay
    pdFALSE,              // One-shot
    (void *)2,
    resetTimerCallback
  );

  drawReady();
}

void loop() {
  // --- 1. Read Serial Input ---
  if (Serial.available() > 0) {
    char c = Serial.read();
    
    // Start or restart session if 's' or 'S' is received
    if ((c == 's' || c == 'S') && currentState == STATE_READY) {
      // Clear out the serial buffer in case of multiple characters
      while (Serial.available()) Serial.read(); 
      
      currentState = STATE_RUNNING;
      countdown_value = 10;
      
      digitalWrite(LED_PIN, HIGH);
      
      // Start both timers
      xTimerStart(periodicTimer, 0);
      xTimerStart(oneShotTimer, 0);
      
      drawRunning();
    }
  }

  // --- 2. Process Task Notifications ---
  uint32_t notifiedValue = 0;
  // Wait for a notification. Timeout is 10ms to keep the serial reader responsive.
  if (xTaskNotifyWait(0x00, ULONG_MAX, &notifiedValue, pdMS_TO_TICKS(10)) == pdTRUE) {
    
    // Check if the tick event bit is set
    if (notifiedValue & EVENT_TICK) {
      if (currentState == STATE_RUNNING) {
        countdown_value--;
        if (countdown_value < 0) countdown_value = 0; // Prevent negative display
        drawRunning();
      }
    }
    
    // Check if the complete event bit is set
    if (notifiedValue & EVENT_COMPLETE) {
      if (currentState == STATE_RUNNING) {
        currentState = STATE_COMPLETE;
        
        digitalWrite(LED_PIN, LOW);
        xTimerStop(periodicTimer, 0); 
        
        drawComplete();
        
        // Start the 1-second countdown to reset the screen
        xTimerStart(resetTimer, 0); 
      }
    }

    // New event handler catches the reset timer firing
    if (notifiedValue & EVENT_RESET) {
      if (currentState == STATE_COMPLETE) {
        currentState = STATE_READY;
        drawReady();
      }
    }
  }
}