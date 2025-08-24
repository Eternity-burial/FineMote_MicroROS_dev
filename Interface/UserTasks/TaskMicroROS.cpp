/*******************************************************************************
* Copyright (c) 2025.
 * IWIN-FINS Lab, Shanghai Jiao Tong University, Shanghai, China.
 * All rights reserved.
 ******************************************************************************/

#include "Scheduler.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

// This task's only dependency is the FreeRTOS semaphore.
extern SemaphoreHandle_t microRosTaskSemaphore;

/**
 * @brief  FineMote task to periodically wake up the micro-ROS worker task.
 *
 * This function runs in the interrupt context of the FineMote scheduler.
 * Its sole responsibility is to release a semaphore at a fixed rate, providing
 * a "heartbeat" for the background micro-ROS task to perform its duties.
 * This decouples the real-time scheduler from the non-real-time communication logic.
 */
void TaskMicroROS() {
    // We will wake up the worker task every 20ms (at a 50Hz rate).
    // This provides a good balance between responsiveness and system load.
    static uint32_t counter = 0;
    if (++counter >= 20) {
        counter = 0;

        if (microRosTaskSemaphore != NULL) {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            // Use the ISR-safe version to give the semaphore.
            xSemaphoreGiveFromISR(microRosTaskSemaphore, &xHigherPriorityTaskWoken);
            // If a higher priority task was woken, request a context switch on ISR exit.
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}

// Export the task to the FineMote scheduler's task list.
TASK_EXPORT(TaskMicroROS);