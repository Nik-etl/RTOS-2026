# FreeRTOS Task Control and States Assignment

## 1. Hardware and Software
* **Board:** ESP32-S3-DevKitC-1 v1.1
* **RGB LED:** GPIO 38
* **IDE:** Arduino IDE 2.x
* **Serial Monitor:** 115200 baud

## 2. Task Design
* **Serial Task:** Runs on Core 1 at Priority 1. It monitors the Serial buffer for commands. It accepts "250", "500", or "1000" to update the global `blinkInterval` variable, and "suspend" or "resume" to directly control the LED task.
* **RGB LED Task:** Runs on Core 1 at Priority 1. It operates completely independently in a continuous loop, turning the LED on and off based on the shared `blinkInterval` variable using `vTaskDelay()`.

## 3. Task Handles
Task handles act as specific reference pointers to task instances created by `xTaskCreatePinnedToCore()`. 
In this application, we create `ledTaskHandle`. When the Serial Task receives a command, it passes this handle to `vTaskSuspend(ledTaskHandle)` and `vTaskResume(ledTaskHandle)` to explicitly target and alter the execution state of the LED Task.

## 4. Suspend / Resume Test
* **suspend:** When "suspend" is entered, the Serial Task calls `vTaskSuspend()`. The LED immediately stops in its current state (either stuck ON or stuck OFF depending on the exact millisecond it was suspended). The Serial Task continues printing and accepting commands flawlessly.
* **resume:** When "resume" is entered, the Serial Task calls `vTaskResume()`. The LED immediately resumes its blinking loop using the most recently set interval. 

## 5. Prediction
| Situation | Prediction | Actual Observation | Match? |
| :--- | :--- | :--- | :--- |
| **LED task running normally** | LED blinks, Serial accepts inputs simultaneously. | LED blinks at 500ms, typing 250 speeds it up smoothly. | Yes |
| **LED task suspended** | LED will freeze, but I can still type new intervals in Serial. | LED froze completely. Serial continued working and accepted a new interval. | Yes |
| **LED task resumed** | LED will continue blinking from where it left off, using any new intervals typed while suspended. | LED immediately resumed blinking. It applied the new interval instantly. | Yes |

## 6. State Analysis

**Situation 1 – Normal operation**
* **Serial Task:** Rapidly alternating between **Blocked** (during `vTaskDelay(50)`) and **Running** (to check `Serial.available()`).
* **RGB LED Task:** Alternating between **Blocked** (waiting for the `blinkInterval` ticks to pass) and **Running** (for the microsecond it takes to toggle the Neopixel).
* *Explanation:* Both tasks spend most of their time in the Blocked state via `vTaskDelay`, allowing the scheduler to seamlessly share the CPU between them when they move to the Ready and Running states.

**Situation 2 – LED task is suspended**
* **Serial Task:** Alternating between **Blocked** and **Running**.
* **RGB LED Task:** **Suspended**.
* *Explanation:* `vTaskSuspend()` explicitly moves the LED task into the Suspended state. The scheduler completely ignores it. It will not execute, even if its delay time expires.

**Situation 3 – LED task is resumed**
* **Serial Task:** Alternating between **Blocked** and **Running**.
* **RGB LED Task:** **Ready** (immediately after resume), then transitions to **Running/Blocked**.
* *Explanation:* `vTaskResume()` removes the LED task from the Suspended state and places it into the Ready state. When the CPU is free, the scheduler selects it, moves it to the Running state, and it continues its blinking loop.

## 7. Reflection
Using task handles allows a FreeRTOS application to control specific task instances dynamically. We use a handle because the scheduler needs an exact reference to know which task's state to modify. 

When `vTaskSuspend(ledTaskHandle)` is called, the scheduler forces the LED task into the **Suspended** state. In this state, the task is completely removed from the scheduler's awareness, it will not consume CPU time regardless of priorities or delays. When `vTaskResume(ledTaskHandle)` is called, the task is pulled out of suspension and placed into the **Ready** state, allowing it to compete for CPU time again. 

Because we designed this as two separate activities rather than one sequential loop, they are independent. The Serial Task can continue working while the LED Task is Suspended because the Serial Task remains eligible for the **Running** state. Suspending one task simply frees up more CPU time for other tasks; it does not block the entire core.

## 8. Independent Task Proof (Screenshots)
*(Place your screenshots here by saving them in the `images` folder)*
* `images/console-log.png`