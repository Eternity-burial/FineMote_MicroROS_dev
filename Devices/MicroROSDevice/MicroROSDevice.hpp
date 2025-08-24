/*******************************************************************************
 * Copyright (c) 2025.
 * IWIN-FINS Lab, Shanghai Jiao Tong University, Shanghai, China.
 * All rights reserved.
 ******************************************************************************/

#ifndef MICROROS_DEVICE_HPP
#define MICROROS_DEVICE_HPP

#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/int32.h> // We will use Int32 for simplicity

/**
 * @class MicroROSDevice
 * @brief Encapsulates all micro-ROS logic for the FineMote application.
 *
 * This class manages the lifecycle of a micro-ROS node, including its
 * initialization, execution, and communication. It is designed as a Singleton
 * and uses a static memory pool for all its operations, ensuring deterministic
 * behavior.
 */
class MicroROSDevice {
public:
    /**
     * @brief Access the singleton instance of the MicroROSDevice.
     * @return A reference to the single instance.
     */
    static MicroROSDevice& GetInstance();

    /**
     * @brief Initializes the micro-ROS node, publisher, and executor.
     *        This method uses a static memory pool and must be called from
     *        a task context (e.g., a FreeRTOS task) after the scheduler has started.
     * @return True if initialization is successful, false otherwise.
     */
    bool initialize();

    /**
     * @brief Performs one cycle of micro-ROS event handling.
     *        This should be called periodically from a task context. It checks for
     *        agent connectivity and spins the executor to handle timers and subscriptions.
     */
    void handle();

    /**
     * @brief Publishes an integer message to the topic.
     * @param data The 32-bit integer data to publish.
     */
    void publish(int32_t data);

private:
    // Private constructor to enforce singleton pattern
    MicroROSDevice();
    ~MicroROSDevice();

    // Prevent copy and assignment
    MicroROSDevice(const MicroROSDevice&) = delete;
    MicroROSDevice& operator=(const MicroROSDevice&) = delete;

    // --- micro-ROS Core Objects ---
    rcl_allocator_t* allocator_; // Pointer to the static allocator
    rclc_support_t support_;
    rcl_node_t node_;
    rclc_executor_t executor_;

    // --- Communication Objects ---
    rcl_publisher_t publisher_;
    std_msgs__msg__Int32 pub_msg_;

    // --- State Machine ---
    enum class State {
        WAITING_FOR_AGENT,
        AGENT_CONNECTED,
        AGENT_DISCONNECTED
    };
    State state_;
};

#endif // MICROROS_DEVICE_HPP