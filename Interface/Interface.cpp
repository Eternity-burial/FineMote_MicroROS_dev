/*******************************************************************************
* Copyright (c) 2023.
 * IWIN-FINS Lab, Shanghai Jiao Tong University, Shanghai, China.
 * All rights reserved.
 ******************************************************************************/

#include "ProjectConfig.h"
#include "DeviceBase.h"
#include "Scheduler.h"


#include "FreeRTOS.h"
#include "task.h"


/*****  RTOS测试 *****/
// void LED1Task(void *pvParameters) {
//     while (true) {
//         HAL_GPIO_TogglePin(LED_G1_GPIO_Port, LED_G1_Pin);
//         vTaskDelay(pdMS_TO_TICKS(300)); // Delay to prevent busy looping
//     }
// }
//
// void LED2Task(void *pvParameters) {
//     while (true) {
//         HAL_GPIO_TogglePin(LED_G2_GPIO_Port, LED_G2_Pin);
//         vTaskDelay(pdMS_TO_TICKS(600)); // Delay to prevent busy looping
//     }
// }
//
// void LED3Task(void *pvParameters) {
//     while (true) {
//         HAL_GPIO_TogglePin(LED_G3_GPIO_Port, LED_G3_Pin);
//         vTaskDelay(pdMS_TO_TICKS(900)); // Delay to prevent busy looping
//     }
// }

/**
 * @brief 用户初始化
 */

#ifdef __cplusplus
extern "C" {
#endif

void Setup() {
    // xTaskCreate(LED1Task, "LED1Task", 128, nullptr, tskIDLE_PRIORITY + 1, nullptr);
    // xTaskCreate(LED2Task, "LED2Task", 128, nullptr, tskIDLE_PRIORITY + 1, nullptr);
    // xTaskCreate(LED3Task, "LED3Task", 128, nullptr, tskIDLE_PRIORITY + 1, nullptr);
}

/**
 * @brief 主循环，优先级低于定时器中断，不确定执行频率
 */
void Loop() {
    // Do something
    HAL_Delay(1000);
}

#ifdef __cplusplus
}
#endif

void MainRTLoop() {
    HAL_IWDG_Refresh(&hiwdg);
    DeviceBase::DevicesHandle();
    FineMoteScheduler();
}

/*****  不要修改以下代码 *****/

#ifdef __cplusplus
extern "C" {
#endif

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM6){
        HAL_IncTick();
    }
    if (htim == &TIM_Control) {
        MainRTLoop();
    }
}

#ifdef __cplusplus
}
#endif
