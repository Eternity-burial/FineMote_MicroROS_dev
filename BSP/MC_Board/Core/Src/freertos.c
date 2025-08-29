/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"


/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usart.h"
#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h> // 建议使用执行器，代码更健壮
#include <uxr/client/transport.h>
#include <rmw_microxrcedds_c/config.h>
#include <rmw_microros/rmw_microros.h>
#include <std_msgs/msg/int32.h>
#include <stdbool.h>
#include <stdio.h> // 用于 printf
#include <string.h> // 用于 memset

#include "allocators.h" // 包含我们自定义的分配器声明
#include "microros_transports.h" // 包含我们自定义的传输层函数声明
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

// 定义 micro-ROS 任务的堆栈大小，可以根据需要调整
#define MICROROS_APP_STACK_SIZE (3000 * 4)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
osThreadId microROS_app_Handle; // 为新的micro-ROS应用任务创建一个句柄
/* USER CODE END Variables */
osThreadId defaultTaskHandle;

  /* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
// 新增一个函数原型，用于micro-ROS的应用任务
void appMain(void const * argument);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
// void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );
//
// /* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
// static StaticTask_t xIdleTaskTCBBuffer;
// static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];
//
// void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
// {
//   *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
//   *ppxIdleTaskStackBuffer = &xIdleStack[0];
//   *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
//   /* place for user code */
// }
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */

  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 1000);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  // 定义并创建micro-ROS应用任务，它将负责实际的ROS节点和发布逻辑
  // 为micro-ROS分配一个较大的栈空间（例如5000字节），防止栈溢出
  osThreadDef(microROS_app, appMain, osPriorityNormal, 0, 2500);
  microROS_app_Handle = osThreadCreate(osThread(microROS_app), NULL);
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const *argument)
{
    /* USER CODE BEGIN StartDefaultTask */
    /* Infinite loop */
  /* USER CODE BEGIN StartDefaultTask */
  // 1. 设置micro-ROS自定义传输层
  //    这里使用了我们自己实现的基于FreeRTOS和HAL库的串口DMA传输函数。
  //    注意：函数名已更正为 `freertos_serial_...` 系列。
  //    通信串口句柄已修改为 `&huart5`，用于和Linux通信。
  rmw_uros_set_custom_transport(
    true,
    (void *) &huart5, // 使用 UART5
    freertos_serial_open,
    freertos_serial_close,
    freertos_serial_write,
    freertos_serial_read);

  // 2. 设置micro-ROS自定义内存分配器
  //    这里将micro-ROS的内存申请指向我们基于FreeRTOS heap_4实现的函数。
  //    注意：函数名已更正为 `__freertos_...` 系列。
  rcl_allocator_t freeRTOS_allocator = rcutils_get_zero_initialized_allocator();
  freeRTOS_allocator.allocate = __freertos_allocate;
  freeRTOS_allocator.deallocate = __freertos_deallocate;
  freeRTOS_allocator.reallocate = __freertos_reallocate;
  freeRTOS_allocator.zero_allocate =  __freertos_zero_allocate;

  // 3. 将我们自定义的分配器设置为rclc的默认分配器
  if (!rcutils_set_default_allocator(&freeRTOS_allocator)) {
    // printf("Error on default allocators (line %d)\n", __LINE__);
    // 在单片机中，如果出现严重错误，可以在这里进入死循环或重启
    while(1);
  }

  // 初始化任务完成，进入无限循环，可以用于系统监控或保持任务存活
  for(;;)
  {
    osDelay(1000);
  }
  /* USER CODE END StartDefaultTask */
}

/**
  * @brief  这个函数是专门为micro-ROS应用创建的新任务。
  *         它包含了所有与ROS节点、发布者、订阅者相关的逻辑。
  * @param  argument: Not used
  * @retval None
  */
void appMain(void const * argument)
{
  // 等待 `StartDefaultTask` 完成初始化
  osDelay(2000);

  rcl_publisher_t publisher;
  std_msgs__msg__Int32 msg;
  rclc_support_t support;
  rcl_allocator_t allocator;
  rcl_node_t node;

  // 获取之前设置好的默认内存分配器
  allocator = rcl_get_default_allocator();

  // 创建初始化选项
  rclc_support_init(&support, 0, NULL, &allocator);

  // 创建节点
  rclc_node_init_default(&node, "cubemx_node", "", &support);

  // 创建发布者
  rclc_publisher_init_default(
    &publisher,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    "cubemx_publisher");

  msg.data = 0;

  // 应用主循环
  for(;;)
  {
    rcl_ret_t ret = rcl_publish(&publisher, &msg, NULL);
    if (ret != RCL_RET_OK)
    {
      // printf("Error publishing (line %d)\n", __LINE__);
      // 发布失败可以在这里处理，例如尝试重新连接
    }

    msg.data++;
    // 发布频率不宜过快，以免饱和，设置为100ms一次
    osDelay(100);
  }
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
