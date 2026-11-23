# ESP32 FreeRTOS Task Starvation & Priority Scheduling

**Purpose:** Identify task starvation in a priority-based preemptive RTOS, observe how a high-priority task without blocking starves lower-priority tasks on an ESP32, and resolve it using periodic delays.

**Board Setup & Wiring:** ESP32 DevKitC (No external wiring required). Serial Monitor set to 115200 baud.

---

## Part A – Understanding the Problem

Consider an ESP32 application with three tasks running on the same core:

| Task | Priority | Behaviour |
| :--- | :--- | :--- |
| **Task_Control** | 3 | Continuously reads a sensor and processes data without a delay |
| **Task_Display** | 2 | Updates the display every 200 ms and then calls `vTaskDelay()` |
| **Task_Log** | 1 | Sends a log message every second and then calls `vTaskDelay()` |

**1. Which tasks may experience starvation? Explain why.**
- **Starved Tasks:** Both `Task_Display` (Priority 2) and `Task_Log` (Priority 1) will starve. Because FreeRTOS uses priority-based preemptive scheduling, `Task_Control` (Priority 3) remains in the Ready/Running state permanently and monopolizes the CPU, never yielding to lower-priority tasks.

**2. Why is it dangerous for a high-priority task to run continuously without blocking?**
- Lower-priority application tasks will never get CPU time, making features like display updates and logging completely unresponsive. Additionally, system tasks such as the Idle task (Priority 0) cannot run to clean up deleted tasks or feed the Task Watchdog Timer (TWDT), which can trigger a watchdog reset and crash the ESP32.

**3. Give two ways to allow lower-priority tasks to get CPU time without changing task priorities.**
1. Call a blocking delay function such as `vTaskDelay()` or `vTaskDelayUntil()` periodically inside the loop.
2. Block on an event or synchronization primitive (such as waiting on a queue, binary semaphore, or task notification) instead of busy-waiting.

---

## Part B – ESP32 Starvation Demo (`Demo2_Starvation.ino`)

### Step 1 – Observe Starvation (`STARVATION_FIX 0`)

**Observations:**
- **Progress of `Task_Low` while `Task_Hog` is `BUSY`:** `Task_Low` is unable to progress and its counter stays frozen.
- **Progress of `Task_Low` while `Task_Hog` is `SLEEP`:** `Task_Low` is able to run and increment its counter because `Task_Hog` is blocked during the sleep phase.
- **What these results show about starvation:** A high-priority task that never enters the Blocked state starves all lower-priority tasks on the same core, allowing them to run only when the high-priority task explicitly blocks.

---

### Step 2 – Enable the Fix (`STARVATION_FIX 1`)

**Comparison & Explanation:**
- **Comparison with Step 1:** Unlike Step 1 where `Task_Low` made zero progress during the `BUSY` period, `Task_Low` now continuously increments its counter even while `Task_Hog` is active.
- **Why `Task_Low` can now make progress:** Calling `vTaskDelay(1)` places `Task_Hog` into the Blocked state for one RTOS tick, allowing the FreeRTOS scheduler to context-switch to `Task_Low` and give it CPU time between `Task_Hog`'s iterations.

---

### Step 3 – Change the Delay (`vTaskDelay(10)`)

**Prediction (Before Running):**
Increasing the delay from `vTaskDelay(1)` to `vTaskDelay(10)` will keep `Task_Hog` in the Blocked state ten times longer per cycle, allowing `Task_Low` to get significantly more CPU time and increment its counter much faster during the `BUSY` period.


**Observation & Explanation:**
During the `BUSY` period, `Task_Low`'s counter increased noticeably faster than it did with `vTaskDelay(1)`. Because `vTaskDelay(10)` blocks `Task_Hog` for 10 RTOS ticks instead of 1 tick each loop, the FreeRTOS scheduler allocates a much larger share of CPU time to `Task_Low` before `Task_Hog` wakes up and preempts it again.

---

## Conclusion / What I Learned
Through this experiment, I learned how FreeRTOS priority-based preemptive scheduling strictly favors the highest-priority Ready task, meaning a single continuous loop can completely starve lower-priority tasks on the same core. Adding even a short blocking call like `vTaskDelay()` forces the high-priority task into the Blocked state periodically, allowing lower-priority tasks to run reliably and balancing CPU time across the system.

## AI Usage
- **Tool used:** Gemini
- **How I used it:** Used to structure the README template