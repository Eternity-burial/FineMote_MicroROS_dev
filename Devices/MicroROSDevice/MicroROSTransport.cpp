// --- START OF FILE MicroROSTransport.cpp ---

/*******************************************************************************
* Copyright (c) 2025.
 * IWIN-FINS Lab, Shanghai Jiao Tong University, Shanghai, China.
 * All rights reserved.
 ******************************************************************************/

#include "MicroROSTransport.hpp"
#include "Bus/UART_Base.hpp"

// FreeRTOS
#include "FreeRTOS.h"
#include "semphr.h"

// micro-ROS
#include <uxr/client/profile/transport/custom/custom_transport.h>
#include <rmw_microros/rmw_microros.h>

// --- 配置 ---
// 定义用于 micro-ROS 通信的 UART ID
constexpr uint8_t MICROROS_UART_ID = 5;

// --- 静态变量 ---
// 用于同步的信号量
static SemaphoreHandle_t tx_complete_semaphore;
static SemaphoreHandle_t rx_complete_semaphore;

// 用于记录中断中接收到的字节数
static volatile uint16_t received_byte_count = 0;

// --- C 风格的传输接口函数 ---

// 打开传输接口
static bool stm32_transport_open(struct uxrCustomTransport* transport) {
    // BSP 的 UART 在实例化时已经通过 CubeMX 初始化，这里无需额外操作。
    return true;
}

// 关闭传输接口
static bool stm32_transport_close(struct uxrCustomTransport* transport) {
    // 通常无需操作，除非需要反初始化硬件
    return true;
}

// 写数据 (阻塞式)
static size_t stm32_transport_write(struct uxrCustomTransport* transport, const uint8_t* buf, size_t len, uint8_t* err) {
    (void)err; // 错误码未使用

    // 1. 启动异步 DMA/IT 发送
    // 注意：BSP_UART::Transmit 的第一个参数需要是 uint8_t*，因此需要 const_cast
    BSP_UART<MICROROS_UART_ID>::GetInstance().Transmit(const_cast<uint8_t*>(buf), len);

    // 2. 等待发送完成信号量
    if (xSemaphoreTake(tx_complete_semaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
        // 发送成功，返回发送的长度
        return len;
    } else {
        // 超时，发送失败
        return 0;
    }
}

// 读数据 (阻塞式)
static size_t stm32_transport_read(struct uxrCustomTransport* transport, uint8_t* buf, size_t len, int timeout, uint8_t* err) {
    (void)err; // 错误码未使用

    // 1. 启动异步 DMA/IT 接收
    BSP_UART<MICROROS_UART_ID>::GetInstance().Receive(buf, len);

    // 2. 等待接收完成信号量
    if (xSemaphoreTake(rx_complete_semaphore, pdMS_TO_TICKS(timeout)) == pdTRUE) {
        // 成功接收到数据，返回实际接收的长度
        return received_byte_count;
    } else {
        // 超时，没有接收到数据
        // 需要手动中止正在进行的接收，以防下次调用 read 时出问题
        HAL_UART_AbortReceive_IT(BSP_UARTList[MICROROS_UART_ID]);
        return 0;
    }
}

// --- 公共 C 函数 ---

void MicroROSTransport_Init() {
    // 创建二进制信号量用于同步
    tx_complete_semaphore = xSemaphoreCreateBinary();
    rx_complete_semaphore = xSemaphoreCreateBinary();
}

void MicroROSTransport_Register() {
    rmw_uros_set_custom_transport(
        true, // 使用帧模式 (framed)
        nullptr, // 自定义状态指针，这里我们不需要
        stm32_transport_open,
        stm32_transport_close,
        stm32_transport_write,
        stm32_transport_read
    );
}

// 发送完成回调，将在 ISR 上下文中被调用
void MicroROSTransport_TxCpltCallback() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(tx_complete_semaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// 接收完成回调，将在 ISR 上下文中被调用
void MicroROSTransport_RxEventCallback(uint16_t size) {
    // 保存从中断中获取的接收字节数
    received_byte_count = size;

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(rx_complete_semaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}//
// Created by Zhangwh on 2025/8/24.
//