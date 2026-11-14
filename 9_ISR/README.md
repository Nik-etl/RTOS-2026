# ESP32-S3 ISR to FreeRTOS Queue

**Purpose:** Transfer hardware timer events from an ISR to a task via a queue and observe behavior.

**Board Setup & Wiring:** ESP32-S3 DevKitC. No external wiring required. Serial monitor set to 115200 baud

## Run A: 20 ms Task Delay

**Output Excerpt:**

![Serial Monitor Output](images/serial-monitor.png)

**Observation:** 
The task processes items (20 ms) faster than the hardware timer generates them (100 ms). The 4-slot queue never fills, no interrupts are dropped, and the sequence numbers print continuously without gaps.

## Run B: 500 ms Task Delay

**Prediction:**
If the task processes items 400ms slower than the ISR generates then some data will be lost while the consumer task is blocked from the delay. The task would be undersampling the Queue.
**Output Excerpt:**

![Serial Monitor Output](images/serial-monitor2.png)


**Explanation of Queue Capacity and Gaps:**
The queue capacity is 4 slots. This allows for safe transfers without too much RAM. 
When the system first wakes up, the consumer task is ready and waiting for a queue reception. 100ms goes by and the interrupt triggers, writes 1 and consumer takes it and prints it. The consumer then is blocked for 500ms, this allows the queue to fill up the 4 slots (200ms -> 2, 300ms -> 3, 400ms -> 4, 500ms -> 5). At 600ms the consumer takes 2 from the queue freeing a space, allowing the ISR to write a 6 into the queue at 600ms. After this however, the queue is now full and has to drop incoming values from 700ms-1000ms (7, 8, 9, 10, skipping 10 too because the ISR has to write the value first before sending the queue). The consumer runs and prints 3 opening a slot in the queue which will be filled by the current counter value: 11. This is why there is a 5 value gap. Technically the value is still accurate but data resolution and timing are not. 

## AI Usage
- **Tool used:** Gemini
- **How I used it:** I used it to format the readme and generate queue code
