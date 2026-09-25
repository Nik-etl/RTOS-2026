Lecture 3 - Task Scheduling and Management

Write approximately 200–300 words explaining your observations.

Your explanation should include:

What is the Running state?
The running state is the state a task is in while it is executing.

What is the Ready state?
The ready state is when a task is possible and waiting to be executed

What is the Blocked state?
The blocked state is explicitly defined and requires some sort of event or condition to be unblocked and then moved to ready state.

Why is Task B initially Blocked?
It is blocked because it is awaiting a notification and that is the way the system was designed to act. 

What happens when Task A calls xTaskNotifyGive()?
Task A sends a notification to Task B which is the conditional task B needs to be unblocked

Why can Task B preempt Task A?
Task B has higher priority therefore it can preempt task A

What happens when Task B has the same priority as Task A?
It will still behave the same. 

Why can Task A continue from where it stopped?
Because each task is configured with its own stack which can store each task's own memory. 


Assignment purpose
The purpose of this assignment is to teach about task management and how the different states can be changed.

Hardware and software
Esp32 S3 devkit 
Arduino IDE 

Your three test scenarios
Task A

Prediction and observation table

Your explanation
