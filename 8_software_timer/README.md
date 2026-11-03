# Assignment: Timed Study Session (FreeRTOS Timers)

## Purpose
Build a 10-second study session timer using an ESP32-S3, an SSD1306 OLED display, and the onboard LED. This project demonstrates the use of three FreeRTOS software timers (2 one-shot and 1 auto-reload) acting together, utilizing short callbacks and task notifications to let an application task safely manage the user interface.

## Hardware & Wiring
- **Board:** ESP32-S3-DevKitC-1
- **Display:** SSD1306 OLED (I²C address `0x3C`)
- **Serial Monitor:** 115200 baud

### Pin Configuration
| Component | ESP32-S3 Pin | Note |
| :--- | :--- | :--- |
| **OLED SDA** | `8` | I²C Data |
| **OLED SCL** | `9` | I²C Clock |
| **Onboard LED** | `6` | `GPIO` |

## Test Results

| Test | Expected result | Observed result | Pass / Fail |
| :--- | :--- | :--- | :--- |
| **Restart the board and wait >10s** | Ready screen remains visible; LED stays off | `Ready screen remains visible; LED stays off ` | `Pass` |
| **Send `s`** | Session starts, countdown appears and LED turns on | Session starts, countdown appears and LED turns on | Pass |
| **Send `s` repeatedly during session** | Original session continues without extending | Original session continues without extending | Pass |
| **Allow the session to finish** | Completion screen appears and LED turns off after ~10 seconds | Completion screen appears and LED turns off after ~10 seconds | Pass |
| **Wait after completion** | Completion screen remains visible | I changed the code and added a 3rd timer so it would return to Ready screen, otherwise it would | Pass |
| **Send `s` again** | A new session starts with the full duration | A new session starts with the full duration | Pass |

*(Note: Small timing differences are normal because of scheduling and display update time.)*

---

## Questions

**1. Why does this system need both a one-shot and an auto-reload timer?**
> Theoretically it does not need the 10 second one-shot timer because it could be replaced with a counter that would count down from the full duration with each auto-reload timer expiration. However for this adaptation we use the autoreload timer to keep track of the countdown and change state, and we need the one shot timer to keep track of the overall session.

**2. Which task executes the timer callbacks?**
> The Timer daemon

**3. Why do OLED updates belong in the application task?**
> OLED updates are time consuming using I2C and could potentially delay the timer daemon, which would cause all other important jobs for the daemon to also be stalled. instead they are executed by the application task which can take as long as it needs without holding up the OS.

**4. When are the timers dormant?**
> The timers are dormant when they are created but not yet running or after they have expired. Basically as long as they exist and are not running.

**5. How does your program prevent extra `s` commands from extending the session?**
> I use an enumerated state variable to keep track of the status of the machine. The program will only accept an input while the machine is in the 'ready' state. This is thanks to the 3rd 'resettimer', without this the code would have to be modified to not accept input when the state is running.