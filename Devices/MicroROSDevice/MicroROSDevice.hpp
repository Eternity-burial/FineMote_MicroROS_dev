/*******************************************************************************
 * Copyright (c) 2025.
 * IWIN-FINS Lab, Shanghai Jiao Tong University, Shanghai, China.
 * All rights reserved.
 ******************************************************************************/

#ifndef MICROROS_DEVICE_HPP
#define MICROROS_DEVICE_HPP

#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/header.h> // 【修改】消息类型改为 Header

// 定义用于 Header 消息中字符串的缓冲区大小
#define STRING_BUFFER_LEN 50

/**
 * @class MicroROSDevice
 * @brief 封装了所有 micro-ROS 逻辑，实现了一个 "ping-pong" 应用。
 */
class MicroROSDevice {
public:
    /**
     * @brief Access the singleton instance of the MicroROSDevice.
     * @return A reference to the single instance.
     */
    static MicroROSDevice& GetInstance();

    /**
     * @brief Initializes the micro-ROS node, publishers, subscribers, and timer.
     * @return True if initialization is successful, false otherwise.
     */
    bool initialize();

    /**
     * @brief Performs one cycle of micro-ROS event handling.
     */
    void handle();

    // 【修改】旧的 publish(int32_t) 方法被移除，因为所有发布逻辑都在定时器和回调中

private:
    // --- 私有构造/析构与单例模式 ---
    MicroROSDevice();
    ~MicroROSDevice();
    MicroROSDevice(const MicroROSDevice&) = delete;
    MicroROSDevice& operator=(const MicroROSDevice&) = delete;

    // --- 回调函数 ---
    // 由于 rclc 是 C 库，回调函数需要是静态的
    static void ping_timer_callback(rcl_timer_t* timer, int64_t last_call_time);
    static void ping_subscription_callback(const void* msgin);
    static void pong_subscription_callback(const void* msgin);

    // --- micro-ROS 核心对象 ---
    rcl_allocator_t* allocator_;
    rclc_support_t support_;
    rcl_node_t node_;
    rclc_executor_t executor_;
    rcl_timer_t ping_timer_;

    // --- 通信对象 (Pub/Sub) ---
    rcl_publisher_t ping_publisher_;
    rcl_publisher_t pong_publisher_;
    rcl_subscription_t ping_subscriber_;
    rcl_subscription_t pong_subscriber_;

    // --- 消息对象 ---
    std_msgs__msg__Header outcoming_ping_msg_;
    std_msgs__msg__Header incoming_ping_msg_;
    std_msgs__msg__Header incoming_pong_msg_;

    // --- 消息内部的字符串缓冲区 ---
    char outcoming_ping_buffer_[STRING_BUFFER_LEN];
    char incoming_ping_buffer_[STRING_BUFFER_LEN];
    char incoming_pong_buffer_[STRING_BUFFER_LEN];

    // --- 应用状态变量 ---
    int device_id_;
    int seq_no_;
    int pong_count_;

    // --- 状态机 ---
    enum class State {
        WAITING_FOR_AGENT,
        AGENT_CONNECTED,
        AGENT_DISCONNECTED
    };
    State state_;
};

#endif // MICROROS_DEVICE_HPP