/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "driver_oled.h"
#include "driver_ir_receiver.h"
#include "driver_led.h"
#include "driver_timer.h"
#include "queue.h"
#include "stdio.h"
#include "semphr.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
// 消息结构 
typedef struct
{
  uint8_t task_id;
  uint32_t value;
  uint32_t time;
} PrintMesage_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* ---------------- 2. 全局队列句柄 ---------------- */
QueueHandle_t xPrintQueue;  // 打印队列句柄
SemaphoreHandle_t xOledDmaSem = NULL;    // 用于DMA完成通知
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */


/* Task1/2/3 的任务句柄, 保存起来供按键任务挂起/恢复 */
 

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void Task(void *pvParameters)
{
  uint8_t task_id = (uint8_t)(uintptr_t)pvParameters;  // 获取任务 ID
  PrintMesage_t msg;  // 定义消息结构体
  msg.task_id = task_id;  // 设置任务 ID
  uint32_t cnt = 0;  // 定义计数器
  // 延时一段时间, 让 OLED 任务先运行, 避免打印队列满
  vTaskDelay(pdMS_TO_TICKS(2000+task_id * 100));

  for (;;)
  {
    cnt++;  // 计数器自增
    msg.value = cnt;
    msg.task_id = task_id;
    msg.time = HAL_GetTick();   // 获取当前系统时间戳

    xQueueSend(xPrintQueue, &msg, portMAX_DELAY);  // 将消息发送到打印队列

    vTaskDelay(pdMS_TO_TICKS(10000 ));  // 延时 3秒 
  }
}
void OLEDTask(void *pvParameters)
{
  PrintMesage_t msg;  // 定义消息结构体
  for (;;)
  {
    // 从打印队列中接收消息
    if (xQueueReceive(xPrintQueue, &msg, portMAX_DELAY) == pdPASS)
    {
      // 拿到数据后，先把内存清空
      OLED_ClearFrameBuffer();
      // 打印任务 ID
      // 注意：y坐标 0, 2, 4, 6 (因为16号字占2页)
      char buffer[32];
      sprintf(buffer, "Task ID: %d", msg.task_id);
      OLED_PrintString(0, 0, buffer);
      // 打印计数值
      sprintf(buffer, "Value: %lu", msg.value);
      OLED_PrintString(0, 2, buffer);
      // 打印时间戳
      sprintf(buffer, "Time: %lu ms", msg.time);
      OLED_PrintString(0,4, buffer);
      // 一次性把这 1024 字节全部通过 DMA 推给屏幕
       OLED_Flush_DMA();
      vTaskDelay(pdMS_TO_TICKS(500));  
    }
  }
}

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

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
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  // 创建打印队列, 队列长度为 10, 每个元素大小为 PrintMesage_t
  xPrintQueue = xQueueCreate(10, sizeof(PrintMesage_t));
  // 创建二值信号量（初值为空）
   xOledDmaSem = xSemaphoreCreateBinary();
  // 创建 5 个任务, 每个任务的优先级为 2, 栈大小为 128
  for (int i = 0; i < 5; i++)
  {
    xTaskCreate(Task, "Task", 128, (void*)i, 2, NULL);
  }
  xTaskCreate(OLEDTask, "OLED", 256, NULL, 1, NULL);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    // OLED_PrintString(0, 0, "Hello World!");
    // HAL_Delay(500);
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    vTaskDelay(pdMS_TO_TICKS(100));      // 用 vTaskDelay 代替 HAL_Delay
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

