# ESP32 FreeRTOS Deadlock & Lock Ordering

**Purpose:** Identify circular wait conditions in a multitasking system, observe a deadlock on an ESP32, and prevent it using consistent mutex lock ordering and timeouts.

**Board Setup & Wiring:** No external wiring required. Serial Monitor set to 115200 baud.

---

## Part A – Understanding the Problem

**1. Which two tasks can cause a deadlock? Explain how they can end up waiting for each other.**
* **Tasks:** task1 with order A then B, then task2 with order B then A, this changes depending on configuration.
* **Explanation:** If the tasks try to take mutexes in opposite order this will create a deadlock because they will each wait for the one mutex the other one currently posseses. When in the same order the other task is blocked before it can take the second mutex.

**2. Which Coffman condition is removed when all tasks acquire mutexes in the same order?**
* The hold and wait condition is removed because the first task to get a mutex will proceed and if the other task tries to preempt it will see that mutex is being used and then will be blocked.

---

## Part B – ESP32 Deadlock Demo (`Demo1_Deadlock.ino`)

### Step 1 – Observe the Deadlock (`DEMO_MODE 0`)

**Observations:**
- **Worker tasks:** Both `Task_1` and `Task_2` stop making progress and freeze permanently once one holds Mutex A and the other holds Mutex B.
- **HEARTBEAT line:** The work counters for `Task_1` and `Task_2` stop incrementing, while the heartbeat tick/uptime counter continues running because the monitor task does not use those mutexes.

---

### Step 2 – Fix the Lock Order (`plan2` Modified)

**Explanation:**
Enforcing a global lock order (always acquiring Mutex A before Mutex B) prevents a circular wait. If `Task_1` holds Mutex A, `Task_2` blocks before taking any mutex, allowing `Task_1` to safely acquire Mutex B, finish its work, and release both locks so `Task_2` can run next.

---

### Step 3 – Timeout and Backoff (`DEMO_MODE 1`)

**Explanation:**
When a task cannot acquire the second mutex before its timeout expires, it aborts the attempt instead of waiting forever. It must immediately release the first mutex it is holding so the other task is no longer blocked and can complete its work. Finally, the random backoff delay staggers when the tasks retry, preventing them from repeatedly colliding in lockstep (livelock).

---

## Conclusion / What I Learned
Through this experiment, I learned how easily a deadlock occurs in FreeRTOS when two tasks acquire shared mutexes in opposite orders, even while unrelated tasks like the heartbeat monitor continue running normally. Defining a strict, consistent lock hierarchy across all tasks eliminates circular wait completely, while finite timeouts paired with releasing held locks and random backoffs provide a reliable fallback when strict ordering is not possible.

## AI Usage
- **Tool used:** Gemini
- **How I used it:** Used to structure the README template and refine explanations of FreeRTOS deadlock concepts.