/*******************************************************************************
 * Copyright (c) 2025.
 * IWIN-FINS Lab, Shanghai Jiao Tong University, Shanghai, China.
 * All rights reserved.
 ******************************************************************************/

#include "MicroROSDevice.hpp"
#include "MicroROSMemoryManager.hpp"
#include <rcl/error_handling.h>
#include <rmw_microros/rmw_microros.h>
#include <cstring> // For memset
#include <rclc/executor.h> // 确保包含了 executor 的头文件

// --- Utility Macros ---
// 一个健壮的用于初始化的错误检查宏。
// 如果 micro-ROS 函数失败，它会打印文件和行号并返回 false。
#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){ return false; }}

// 一个用于运行时操作的较宽松的检查宏。
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
    publisher_(),
    pub_msg_(),
    state_(State::WAITING_FOR_AGENT)
{
    // 构造函数保持轻量。
}

MicroROSDevice::~MicroROSDevice()
{
    // 仅在节点成功连接后才执行清理。
    if (state_ == State::AGENT_CONNECTED) {
        // 在析构函数中可以忽略返回值。
        (void)rcl_publisher_fini(&publisher_, &node_);
        (void)rcl_node_fini(&node_);
        (void)rclc_support_fini(&support_);
    }
}


// --- 公共方法 ---

bool MicroROSDevice::initialize()
{
    // 1. 初始化我们的自定义静态内存管理器
    MicroROSMemoryManager::initialize();
    allocator_ = MicroROSMemoryManager::getAllocator();
    if (allocator_ == nullptr) {
        return false; // 内存管理器初始化失败
    }

    // 2. 初始化 micro-ROS 支持库 (support library)
    // 【修正】不再需要手动 memset，rclc_support_init 会完成所有初始化工作。
    RCCHECK(rclc_support_init(&support_, 0, NULL, allocator_));

    // 3. 创建 ROS 节点 (Node)
    // (这部分原本就是正确的)
    node_ = rcl_get_zero_initialized_node();
    RCCHECK(rclc_node_init_default(&node_, "finemote_node", "", &support_));

    // 4. 创建发布者 (Publisher)
    // (这部分原本就是正确的)
    publisher_ = rcl_get_zero_initialized_publisher();
    const rosidl_message_type_support_t* type_support =
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32);

    RCCHECK(rclc_publisher_init_default(
        &publisher_,
        &node_,
        type_support,
        "/finemote_int32_publisher"
    ));

    // 5. 初始化执行器 (Executor)
    // 【修正】使用官方宏来获取一个零初始化的执行器，而不是使用不安全的 memset。
    // 这是导致 HardFault 的关键原因。
    executor_ = rclc_executor_get_zero_initialized_executor();

    // 我们没有订阅或定时器，所以理论上1个句柄就够了。
    // 这可能是为了处理内部节点事件。这是一个安全默认值。
    const size_t num_handles = 1;
    RCCHECK(rclc_executor_init(&executor_, &support_.context, num_handles, allocator_));

    // 设置初始状态
    state_ = State::WAITING_FOR_AGENT;

    return true;
}

void MicroROSDevice::handle()
{
    switch (state_)
    {
        case State::WAITING_FOR_AGENT:
        {
            const int timeout_ms = 100;
            if (rmw_uros_ping_agent(timeout_ms, 1) == RMW_RET_OK) {
                state_ = State::AGENT_CONNECTED;
            }
            break;
        }

        case State::AGENT_CONNECTED:
        {
            const int timeout_ms = 10;
            if (rmw_uros_ping_agent(timeout_ms, 1) != RMW_RET_OK) {
                state_ = State::AGENT_DISCONNECTED;
            } else {
                // 执行器没有任何回调要处理，但调用它以处理后台DDS事件是好习惯。
                // 超时设为0使其成为非阻塞调用。
                RCSOFTCHECK(rclc_executor_spin_some(&executor_, 0));
            }
            break;
        }

        case State::AGENT_DISCONNECTED:
        {
            // 这里可以添加一些清理或重置逻辑，目前只是简单地回到等待状态。
            state_ = State::WAITING_FOR_AGENT;
            break;
        }
    }
}

void MicroROSDevice::publish(int32_t data)
{
    if (state_ == State::AGENT_CONNECTED)
    {
        pub_msg_.data = data;
        RCSOFTCHECK(rcl_publish(&publisher_, &pub_msg_, NULL));
    }
}