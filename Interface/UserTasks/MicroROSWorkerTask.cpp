// --- START OF FILE MicroROSWorkerTask.cpp ---

/*******************************************************************************
 * Copyright (c) 2025.
 * IWIN-FINS Lab, Shanghai Jiao Tong University, Shanghai, China.
 * All rights reserved.
 ******************************************************************************/

#include "MicroROSDevice.hpp"
#include "MicroROSTransport.hpp" // 【修改】1. 包含 micro-ROS 传输层的头文件
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

// The semaphore used to communicate with the FineMote ISR task.
extern SemaphoreHandle_t microRosTaskSemaphore;

/**
 * @brief  The main worker task for all micro-ROS operations.
 * @param  argument Not used.
 *
 * This FreeRTOS task is the designated context for all micro-ROS activities.
 * It remains blocked, waiting for a semaphore released by the high-frequency
 * TaskMicroROS. Upon waking, it initializes the micro-ROS stack (if not already
 * done) and handles all communication and event processing. This isolates all
 * potentially blocking and long-running ROS operations from the real-time
 * interrupt context.
 */
extern "C" void MicroROSWorkerTask(void *argument)
{
    // Get the singleton instance of our micro-ROS device wrapper.
    auto& uros_device = MicroROSDevice::GetInstance();
    bool is_initialized = false;
    int32_t counter = 0;

    // Main task loop
    for (;;)
    {
        // Block indefinitely until woken up by the TaskMicroROS ISR.
        if (xSemaphoreTake(microRosTaskSemaphore, portMAX_DELAY) == pdTRUE)
        {
            // --- One-time Initialization ---
            if (!is_initialized) {
                // 【修改】2. 在初始化 micro-ROS 核心之前，先设置好底层传输
                // 2.1 初始化传输层所需的 FreeRTOS 资源 (信号量)
                MicroROSTransport_Init();
                // 2.2 向 micro-ROS RMW 层注册我们的自定义收发函数
                MicroROSTransport_Register();

                // 2.3 现在可以安全地尝试初始化 micro-ROS 设备了
                if (uros_device.initialize()) {
                    is_initialized = true;
                } else {
                    // Initialization failed. This is a critical error.
                    // We can enter a loop, toggle an error LED, or simply delete the task.
                    // For now, we spin forever.
                    while (1) {
                        // Perhaps toggle an error LED here
                        vTaskDelay(pdMS_TO_TICKS(500));
                    }
                }
            }

            // --- Periodic Operations ---
            if (is_initialized) {
                // 1. Handle the micro-ROS state machine (agent connection, executor spin)
                uros_device.handle();

                // 2. Publish data periodically.
                // This is a simple example of application logic.
                uros_device.publish(counter++);
            }
        }
    }
}