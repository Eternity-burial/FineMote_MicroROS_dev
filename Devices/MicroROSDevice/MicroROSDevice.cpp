/*******************************************************************************
 * Copyright (c) 2025.
 * IWIN-FINS Lab, Shanghai Jiao Tong University, Shanghai, China.
 * All rights reserved.
 ******************************************************************************/

#include "MicroROSDevice.hpp"
#include "MicroROSMemoryManager.hpp"
#include <rcl/error_handling.h>
#include <rmw_microros/rmw_microros.h>
#include <rclc/executor.h>

// 【新增】包含 C 标准库用于字符串处理和随机数
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime> // For srand()

// --- Utility Macros ---
#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){ return false; }}
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){}}

// --- 单例实例化 ---
MicroROSDevice& MicroROSDevice::GetInstance()
{
    static MicroROSDevice instance;
    return instance;
}

// --- 构造函数 & 析构函数 ---
MicroROSDevice::MicroROSDevice() :
    allocator_(nullptr),
    support_(),
    node_(),
    executor_(),
    ping_timer_(),
    ping_publisher_(),
    pong_publisher_(),
    ping_subscriber_(),
    pong_subscriber_(),
    outcoming_ping_msg_(),
    incoming_ping_msg_(),
    incoming_pong_msg_(),
    device_id_(0),
    seq_no_(0),
    pong_count_(0),
    state_(State::WAITING_FOR_AGENT)
{
    // 构造函数保持轻量。
}

MicroROSDevice::~MicroROSDevice()
{
    if (state_ == State::AGENT_CONNECTED) {
        // 按照创建的相反顺序销毁资源
        (void)rcl_subscription_fini(&pong_subscriber_, &node_);
        (void)rcl_subscription_fini(&ping_subscriber_, &node_);
        (void)rcl_publisher_fini(&pong_publisher_, &node_);
        (void)rcl_publisher_fini(&ping_publisher_, &node_);
        (void)rcl_timer_fini(&ping_timer_);
        (void)rcl_node_fini(&node_);
        (void)rclc_support_fini(&support_);
    }
}

// --- 公共方法 ---
bool MicroROSDevice::initialize()
{
    // 1. 初始化内存管理器
    MicroROSMemoryManager::initialize();
    allocator_ = MicroROSMemoryManager::getAllocator();
    if (allocator_ == nullptr) { return false; }

    // 2. 初始化 Support
    RCCHECK(rclc_support_init(&support_, 0, NULL, allocator_));

    // 3. 初始化 Node
    node_ = rcl_get_zero_initialized_node();
    RCCHECK(rclc_node_init_default(&node_, "pingpong_node_cpp", "", &support_));

    // 4. 获取消息类型支持
    const rosidl_message_type_support_t* type_support =
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Header);

    // 5. 初始化 Publishers
    ping_publisher_ = rcl_get_zero_initialized_publisher();
    RCCHECK(rclc_publisher_init_default( // Reliable QoS
        &ping_publisher_, &node_, type_support, "/microROS/ping"));

    pong_publisher_ = rcl_get_zero_initialized_publisher();
    RCCHECK(rclc_publisher_init_best_effort( // Best-Effort QoS
        &pong_publisher_, &node_, type_support, "/microROS/pong"));

    // 6. 初始化 Subscribers
    ping_subscriber_ = rcl_get_zero_initialized_subscription();
    RCCHECK(rclc_subscription_init_best_effort(
        &ping_subscriber_, &node_, type_support, "/microROS/ping"));

    pong_subscriber_ = rcl_get_zero_initialized_subscription();
    RCCHECK(rclc_subscription_init_best_effort(
        &pong_subscriber_, &node_, type_support, "/microROS/pong"));

    // 7. 初始化 Timer (每2秒触发一次)
    // 【修正】使用 rcl_... 而不是 rclc_...
    ping_timer_ = rcl_get_zero_initialized_timer();
    RCCHECK(rclc_timer_init_default(
        &ping_timer_, &support_, RCL_MS_TO_NS(2000), ping_timer_callback));

    // 8. 初始化 Executor
    executor_ = rclc_executor_get_zero_initialized_executor();
    const size_t num_handles = 3;
    RCCHECK(rclc_executor_init(&executor_, &support_.context, num_handles, allocator_));

    // 9. 将回调添加到 Executor
    RCCHECK(rclc_executor_add_timer(&executor_, &ping_timer_));
    RCCHECK(rclc_executor_add_subscription(
        &executor_, &ping_subscriber_, &incoming_ping_msg_,
        &MicroROSDevice::ping_subscription_callback, ON_NEW_DATA));
    RCCHECK(rclc_executor_add_subscription(
        &executor_, &pong_subscriber_, &incoming_pong_msg_,
        &MicroROSDevice::pong_subscription_callback, ON_NEW_DATA));

    // 10. 为消息中的字符串手动分配缓冲区
    outcoming_ping_msg_.frame_id.data = outcoming_ping_buffer_;
    outcoming_ping_msg_.frame_id.capacity = STRING_BUFFER_LEN;

    incoming_ping_msg_.frame_id.data = incoming_ping_buffer_;
    incoming_ping_msg_.frame_id.capacity = STRING_BUFFER_LEN;

    incoming_pong_msg_.frame_id.data = incoming_pong_buffer_;
    incoming_pong_msg_.frame_id.capacity = STRING_BUFFER_LEN;

    // 11. 初始化应用状态
    srand(time(NULL)); // 初始化随机数种子
    device_id_ = rand();
    state_ = State::WAITING_FOR_AGENT;

    return true;
}

void MicroROSDevice::handle()
{
    switch (state_)
    {
        case State::WAITING_FOR_AGENT:
        {
            if (rmw_uros_ping_agent(100, 1) == RMW_RET_OK) {
                state_ = State::AGENT_CONNECTED;
            }
            break;
        }
        case State::AGENT_CONNECTED:
        {
            if (rmw_uros_ping_agent(10, 1) != RMW_RET_OK) {
                state_ = State::AGENT_DISCONNECTED;
            } else {
                RCSOFTCHECK(rclc_executor_spin_some(&executor_, 0));
            }
            break;
        }
        case State::AGENT_DISCONNECTED:
        {
            state_ = State::WAITING_FOR_AGENT;
            break;
        }
    }
}

// --- 静态回调函数实现 ---

void MicroROSDevice::ping_timer_callback(rcl_timer_t* timer, int64_t last_call_time)
{
    (void)last_call_time;
    if (timer == nullptr) return;

    auto& self = GetInstance();

    self.seq_no_ = rand();
    snprintf(self.outcoming_ping_msg_.frame_id.data,
             self.outcoming_ping_msg_.frame_id.capacity,
             "%d_%d", self.seq_no_, self.device_id_);
    self.outcoming_ping_msg_.frame_id.size = strlen(self.outcoming_ping_msg_.frame_id.data);

    // 【修正】使用 micro-ROS 的时间 API
    int64_t nanosecs = rmw_uros_epoch_nanos();
    self.outcoming_ping_msg_.stamp.sec = nanosecs / 1000000000;
    self.outcoming_ping_msg_.stamp.nanosec = nanosecs % 1000000000;

    self.pong_count_ = 0;
    // 【修正】添加 (void) 转换来处理警告
    (void)rcl_publish(&self.ping_publisher_, &self.outcoming_ping_msg_, NULL);
}

void MicroROSDevice::ping_subscription_callback(const void* msgin)
{
    auto& self = GetInstance();
    const std_msgs__msg__Header* msg = static_cast<const std_msgs__msg__Header*>(msgin);

    if (strcmp(self.outcoming_ping_msg_.frame_id.data, msg->frame_id.data) != 0) {
        // 【修正】添加 (void) 转换来处理警告
        (void)rcl_publish(&self.pong_publisher_, msg, NULL);
    }
}

void MicroROSDevice::pong_subscription_callback(const void* msgin)
{
    auto& self = GetInstance();
    const std_msgs__msg__Header* msg = static_cast<const std_msgs__msg__Header*>(msgin);

    if (strcmp(self.outcoming_ping_msg_.frame_id.data, msg->frame_id.data) == 0) {
        self.pong_count_++;
    }
}