# Femtiki OS

This is still a project in the works.

# Priorities
64 priority levels are supported.

# Scheduler
A time-slice interrupt takes care of scheduling in basically a round-robin fashion. The exception is if the task priority is greater than 59 it will not automatically select another to run. It will remain in the same task until the task gives up its time slice. If the task priority is equal to 63 then the
scheduler will not even do the timeout code. The idea is to allow for tasks that need to procede without being interrupted.

# Ready Queues
The latest worked on version of the code expects the ready queues to be in hardware. There is a separate hardware queue for each priority level. Hardware then presents the highest priority task as output. From a software perspective determining which task to run is as simple as loading the top priority task from a hardware register.

# Configuration
There is a config.xxx file containing configuration constants for the OS. It allows setting items like the number of possible tasks and messages without having to dive into the code.

