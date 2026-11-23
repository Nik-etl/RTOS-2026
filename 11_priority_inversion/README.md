# ESP32 FreeRTOS Priority Inversion & Priority Inheritance

**Purpose:** Observe unbounded priority inversion using a binary semaphore, prevent it using a FreeRTOS mutex with priority inheritance, and measure the waiting time of a high-priority task across different workloads.

**Board Setup & Simulation:** ESP32-S3 DevKitC. No external wiring required. Serial Monitor set to 115200 baud.

---

## Part A – Predict the Behaviour

**1. With a binary semaphore, what happens after `Task_H` requests the lock? Explain how `Task_M` affects `Task_L`.**
- When `Task_H` requests the binary semaphore at 100 ms, it immediately enters the Blocked state because `Task_L` still holds the semaphore. `Task_L` continues running until 150 ms, when `Task_M` (Priority 2) becomes ready and preempts `Task_L` (Priority 1). Because `Task_M` runs for 1000 ms without needing the lock, `Task_L` is delayed from finishing its remaining work and releasing the semaphore, forcing `Task_H` to wait for both `Task_M` and `Task_L`.

**2. Simple timeline (Binary Semaphore configuration):**
```text
0 ms     : Task_L (Priority 1) takes lock and starts work section 1
100 ms   : Task_H (Priority 3) starts, requests lock -> BLOCKED; Task_L resumes
150 ms   : Task_M (Priority 2) starts -> preempts Task_L and runs for 1000 ms
~1150 ms : Task_M finishes; Task_L resumes remaining work sections
~1350 ms : Task_L releases lock -> Task_H acquires lock and runs
~1450 ms : Task_H finishes
```

**3. What changes when a mutex with priority inheritance is used? Describe or draw the new timeline.**
- **Change:** When `Task_H` blocks waiting for the mutex held by `Task_L`, FreeRTOS temporarily raises `Task_L`'s priority to match `Task_H` (Priority 3). This prevents `Task_M` (Priority 2) from preempting `Task_L` at 150 ms, allowing `Task_L` to finish its critical section quickly and release the mutex for `Task_H`.
- **New Timeline:**
```text
0 ms   : Task_L (Priority 1) takes mutex and starts work section 1
100 ms : Task_H (Priority 3) requests mutex -> BLOCKED; Task_L inherits Priority 3
150 ms : Task_M (Priority 2) becomes ready, but cannot preempt Task_L (Priority 3)
300 ms : Task_L finishes work, releases mutex, returns to Priority 1 -> Task_H acquires mutex and runs
400 ms : Task_H finishes -> Task_M runs for 1000 ms
```

**4. Which task temporarily inherits a higher priority? What priority does it inherit?**
- **Task:** `Task_L`
- **Inherited Priority:** Priority 3 (the priority of `Task_H`)

---

## Part B – Run and Measure the Demo (`Demo3_Priority_Inversion.ino`)

### Step 1 – Binary Semaphore (`USE_MUTEX 0`)

**Recorded Values:**
- **`Task_H` waiting time:** 1202 ms
- **`Task_L` printed priority:** 1
- **Completion order:** `Task_M` -> `Task_L` -> `Task_H`

---

### Step 2 – Mutex with Priority Inheritance (`USE_MUTEX 1`)

**Recorded Values:**
- **`Task_H` waiting time:** 211ms
- **`Task_L` printed priority:** 3
- **Completion order:** `Task_L` -> `Task_H` -> `Task_M`

**Comparison with Part A Predictions:**
The measured results match the Part A predictions. With the binary semaphore, `Task_L` remained at priority 1, got preempted by `Task_M`, and caused `Task_H` to wait for over a second (~1200 ms). With the mutex enabled, `Task_L` inherited priority 3, finished without interruption from `Task_M`, and reduced `Task_H`'s waiting time to only the remaining duration of `Task_L`'s critical section (~200 ms).

---

### Step 3 – Change the Workload (`busyWorkMs(2000)`)

**Prediction (Before Changing Code):**
Increasing `Task_M`'s workload from 1000 ms to 2000 ms will add ~1000 ms to `Task_H`'s waiting time when using the binary semaphore (`USE_MUTEX 0`), because `Task_M` preempts `Task_L` for twice as long. Under the mutex (`USE_MUTEX 1`), `Task_H`'s waiting time will stay unchanged (~200 ms) because `Task_M` still cannot preempt `Task_L`.

**Measured Results (`busyWorkMs(2000)`):**
- **`Task_H` waiting time (`USE_MUTEX 0`):** 2200ms
- **`Task_H` waiting time (`USE_MUTEX 1`):** 211ms

**Comparison & Explanation:**
The measurements confirmed the prediction: `Task_H`'s waiting time increased by ~1000 ms in binary semaphore mode while remaining constant in mutex mode. With a binary semaphore, `Task_M` preempts `Task_L` and creates unbounded priority inversion directly proportional to `Task_M`'s execution time. With a mutex, priority inheritance keeps `Task_L` at priority 3 until it releases the lock, completely isolating `Task_H` from `Task_M`'s longer workload.

---

## Part C – Explain What You Observed

**1. Why does `Task_H` wait longer when a binary semaphore is used?**
- With a binary semaphore, `Task_L` keeps its low **priority** (1) while holding the lock, which allows the medium-priority `Task_M` (priority 2) to **preempt** `Task_L`. Because `Task_L` cannot run until `Task_M` finishes, `Task_H` is forced to wait for both `Task_M` and `Task_L` before it can acquire the semaphore.

**2. Explain how `Task_L` inherits the priority of `Task_H` when a mutex is used.**
- When `Task_H` attempts to take a mutex that is currently owned by `Task_L`, the FreeRTOS kernel detects that a higher-priority task is blocked on a lower-priority holder. To resolve this, `Task_L` temporarily **inherits** `Task_H`'s **priority** (3) until `Task_L` releases the mutex, at which point it reverts to its original priority.

**3. Why can `Task_M` no longer preempt `Task_L` while `Task_L` has inherited priority 3?**
- FreeRTOS always schedules the highest-priority Ready task to run on the core. While `Task_L` **inherits** **priority** 3, its effective priority is higher than `Task_M`'s priority 2, so `Task_M` cannot **preempt** `Task_L` and must remain in the Ready queue until both `Task_L` and `Task_H` finish.

**4. What is the main difference between a binary semaphore and a mutex in this demonstration?**
- Although both act as locks in this demo, a mutex includes ownership and **priority** inheritance: when a high-priority task blocks on it, the holder **inherits** that higher **priority** so medium-priority tasks cannot **preempt** it. A binary semaphore has no concept of ownership or priority inheritance, leaving the system vulnerable to unbounded priority inversion.

---

## Conclusion / What I Learned
Through this experiment, I learned how priority inversion occurs when a medium-priority task preempts a low-priority task holding a shared resource needed by a higher priority task. Using a FreeRTOS mutex instead of a binary semaphore enables priority inheritance, making the resource holder temporarily runs at the high-priority task's level so critical sections finish  without interference from unrelated medium-priority tasks.

## AI Usage
- **Tool used:** Gemini
- **How I used it:** Used to structure the README template and refine explanations