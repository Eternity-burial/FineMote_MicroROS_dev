// --- START OF FILE MicroROSTransport.hpp ---

/*******************************************************************************
* Copyright (c) 2025.
 * IWIN-FINS Lab, Shanghai Jiao Tong University, Shanghai, China.
 * All rights reserved.
 ******************************************************************************/

#ifndef FINEMOTE_MICROROS_TRANSPORT_HPP
#define FINEMOTE_MICROROS_TRANSPORT_HPP

#include <cstdint>
#ifdef __cplusplus
extern "C" {
#endif

    /**
     * @brief 初始化 micro-ROS 传输层所需的资源 (例如信号量)。
     *        必须在 FreeRTOS 调度器启动后，但在使用传输层之前调用。
     */
    void MicroROSTransport_Init();

    /**
     * @brief 向 micro-ROS RMW 层注册我们的自定义传输函数。
     *        必须在调用任何 rclc 初始化函数之前调用。
     */
    void MicroROSTransport_Register();

    /**
     * @brief micro-ROS 传输层使用的 UART5 的发送完成回调。
     *        这个函数必须从全局的 HAL_UART_TxCpltCallback 中被调用。
     */
    void MicroROSTransport_TxCpltCallback();

    /**
     * @brief micro-ROS 传输层使用的 UART5 的接收事件回调。
     * @param size 接收到的字节数。
     *        这个函数必须从全局的 HAL_UARTEx_RxEventCallback 中被调用。
     */
    void MicroROSTransport_RxEventCallback(uint16_t size);

#ifdef __cplusplus
}
#endif

#endif // FINEMOTE_MICROROS_TRANSPORT_HPP