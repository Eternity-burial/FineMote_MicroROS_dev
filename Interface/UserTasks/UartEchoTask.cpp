/*******************************************************************************
* Copyright (c) 2023.
 * IWIN-FINS Lab, Shanghai Jiao Tong University, Shanghai, China.
 * All rights reserved.
 ******************************************************************************/

#include "Scheduler.h"      // 包含 FineMote 调度器头文件，为了使用 TASK_EXPORT
#include "Bus/UART_Base.hpp" // 包含框架的 UART 核心头文件

/**
 * @brief  上层数据处理回调函数
 * @param  data 指向一个已填满数据的缓冲区的指针
 * @param  size 实际接收到的数据长度
 * @note   这个函数将在UART接收到一帧数据后，由FineMote的UART框架自动调用。
 */
static void OnUart5DataReceived(uint8_t* data, size_t size)
{
    // 检查是否收到了有效数据
    if (size > 0)
    {
        // 将接收到的数据原封不动地发回。
        // 我们通过 UART_Base 的单例模式获取 UART5 的实例，并调用其 Transmit 方法。
        // Transmit 方法会将数据放入发送队列，框架会在后台自动处理发送。
        UART_Base<5>::GetInstance().Transmit(data, size);
    }
}


/**
 * @brief  UART 回显测试任务
 * @note   这个函数会被 FineMoteScheduler 高频调用。
 *         我们的主要工作都在初始化阶段完成，循环阶段无事可做。
 */
void UartEchoTask()
{
    // 使用静态变量确保初始化代码只执行一次
    static bool is_initialized = false;
    if (!is_initialized)
    {
        // 关键步骤：创建一个 UARTBuffer 的静态实例。
        // 模板参数 <5, 128> 表示：
        //   - 5: 我们正在为 UART5 服务。
        //   - 128: 我们为双缓冲区的每一个缓冲区分配 128 字节的大小。
        // 构造函数参数 OnUart5DataReceived 是我们将要注册的回调函数。
        //
        // 这个对象的创建过程会自动完成以下工作：
        // 1. 将自身（缓冲区信息和回调函数）绑定到 UART_Base<5>。
        // 2. 启动 UART5 的第一次 DMA + IDLE 中断接收。
        static UARTBuffer<5, 128> uart5_buffer(OnUart5DataReceived);

        // 标记初始化已完成
        is_initialized = true;
    }

    // 在这个事件驱动的设计中，主循环部分不需要执行任何操作。
    // 所有的接收和发送逻辑都由中断和回调函数驱动。
    // UartEchoTask 函数每次被调度时，会快速执行完这个空的主体然后返回。
}


// 使用 FineMote 框架的宏，将 UartEchoTask 函数注册到调度器中
TASK_EXPORT(UartEchoTask);