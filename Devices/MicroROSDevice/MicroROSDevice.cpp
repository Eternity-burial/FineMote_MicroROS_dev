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

// --- Utility Macros ---
// A robust error-checking macro for initialization.
// If a micro-ROS function fails, it prints the file and line and returns false.
#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){ return false; }}

// A softer check for runtime operations.
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){}}


// --- Singleton Instantiation ---

MicroROSDevice& MicroROSDevice::GetInstance()
{
    static MicroROSDevice instance;
    return instance;
}


// --- Constructor & Destructor ---

MicroROSDevice::MicroROSDevice() :
    allocator_(nullptr),
    support_(),
    node_(),
    executor_(),
    publisher_(),
    pub_msg_(),
    state_(State::WAITING_FOR_AGENT)
{
    // Constructor remains lightweight.
}

MicroROSDevice::~MicroROSDevice()
{
    // Perform cleanup only if the node was successfully connected.
    if (state_ == State::AGENT_CONNECTED) {
        // We can ignore the return values in the destructor.
        (void)rcl_publisher_fini(&publisher_, &node_);
        (void)rcl_node_fini(&node_);
        (void)rclc_support_fini(&support_);
    }
}


// --- Public Methods ---

bool MicroROSDevice::initialize()
{
    // 1. Initialize our custom static memory manager
    MicroROSMemoryManager::initialize();
    allocator_ = MicroROSMemoryManager::getAllocator();
    if (allocator_ == nullptr) {
        return false; // Memory manager failed to initialize
    }

    // 2. Initialize micro-ROS support library
    // [FIX] Replace non-existent function with manual zero-initialization using memset.
    memset(&support_, 0, sizeof(rclc_support_t));
    RCCHECK(rclc_support_init(&support_, 0, NULL, allocator_));

    // 3. Create the ROS Node
    node_ = rcl_get_zero_initialized_node();
    RCCHECK(rclc_node_init_default(&node_, "finemote_node", "", &support_));

    // 4. Create the Publisher
    publisher_ = rcl_get_zero_initialized_publisher();
    const rosidl_message_type_support_t* type_support =
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32);

    RCCHECK(rclc_publisher_init_default(
        &publisher_,
        &node_,
        type_support,
        "/finemote_int32_publisher"
    ));

    // 5. Initialize the Executor
    // [FIX] Replace non-existent function with manual zero-initialization.
    memset(&executor_, 0, sizeof(rclc_executor_t));
    // We have no subscriptions or timers, so only 1 handle is needed.
    // This might be for internal node events. A safe default.
    const size_t num_handles = 1;
    RCCHECK(rclc_executor_init(&executor_, &support_.context, num_handles, allocator_));

    // Set the initial state
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
                // The executor doesn't have any callbacks to handle, but spinning it
                // is good practice for processing background DDS events.
                // A timeout of 0 makes it non-blocking.
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

void MicroROSDevice::publish(int32_t data)
{
    if (state_ == State::AGENT_CONNECTED)
    {
        pub_msg_.data = data;
        RCSOFTCHECK(rcl_publish(&publisher_, &pub_msg_, NULL));
    }
}