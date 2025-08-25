/*******************************************************************************
* Copyright (c) 2025.
 * IWIN-FINS Lab, Shanghai Jiao Tong University, Shanghai, China.
 * All rights reserved.
 ******************************************************************************/

#include "Bus/UART_Base.hpp"
#include "MicroROSTransport.hpp" //  包含 micro-ROS 传输层的头文件
#include <cstdint>
#ifdef __cplusplus
extern "C" {
#endif

// 发送完成中断回调函数
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == BSP_UARTList[5]->Instance) {
        // 如果是，则调用 micro-ROS 传输层的发送完成回调
        MicroROSTransport_TxCpltCallback();
    } else {
        // 否则，执行原来的通用回调逻辑，不影响其他 UART 的功能
        FineMoteAux_UART<>::OnTxComplete(huart);
    }}

// 接收中断回调函数
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    uint16_t receivedSize = huart->RxXferSize;
    FineMoteAux_UART<>::OnRxComplete(huart, receivedSize);
}

// 出错中断回调函数
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == BSP_UARTList[5]->Instance) {
        // 如果 UART5 出错，也通知传输层接收“完成”，但字节数为 0
        MicroROSTransport_RxEventCallback(0);
    } else {
        // 其他 UART 的错误处理
        FineMoteAux_UART<>::OnRxComplete(huart, 0);
    }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size) {
    if (huart->Instance == BSP_UARTList[5]->Instance) {
        // 如果是，则调用 micro-ROS 传输层的接收事件回调，并传入接收到的字节数
        MicroROSTransport_RxEventCallback(size);
    } else {
        // 否则，执行原来的通用回调逻辑
        FineMoteAux_UART<>::OnRxComplete(huart, size);
    }
}

#ifdef __cplusplus
}
#endif
