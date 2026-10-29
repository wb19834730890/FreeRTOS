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
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* LED 任务栈大小, 单位: 字(StackType_t = 4 字节), 256 字 = 1KB */
#define LED_TASK_STACK_SIZE 256
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
/* LED 任务使用"静态创建", 栈和 TCB 由用户提供, 不占 FreeRTOS 堆 */
static StackType_t  g_LedTaskStack[LED_TASK_STACK_SIZE];
static StaticTask_t g_LedTaskTCB;
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
void MyTask(void *argument)
{
  while(1)
  {
    // OLED_PrintString(0, 3, "OLED Ready");
    // HAL_Delay(500);
      OLED_Test();
  }
}

struct OLEDTaskParams
{
  uint8_t x;
  uint8_t y;
  char *str;
};

static struct OLEDTaskParams g_OLEDTaskParams1 = {0, 0, "Task1"};
static struct OLEDTaskParams g_OLEDTaskParams2 = {0, 3, "Task2"};
static struct OLEDTaskParams g_OLEDTaskParams3 = {0, 6, "Task3"};   
static int oled_canuse = 1;  /* OLED 是否可用, 0-不可用, 1-可用 */

void OLEDPrintTask(void *params)
{
  struct OLEDTaskParams *pParams = (struct OLEDTaskParams *)params;
  uint32_t cnt = 0;
  int len;
  while(1)
  {
    if (oled_canuse)
    {
      oled_canuse = 0;  /* 占用 OLED */
      len  = OLED_PrintString(pParams->x,       pParams->y, pParams->str);
      len += OLED_PrintString(pParams->x + len, pParams->y, ":");
      OLED_PrintSignedVal(pParams->x + len, pParams->y, cnt++);
      oled_canuse = 1;  /* 释放 OLED */
    }
    osDelay(500);  /* 用 osDelay 让出 CPU, 不能用 HAL_Delay()/mdelay 忙等 */
    
  }
}
/**
  * @brief  红外接收任务: 轮询按键码并显示到 OLED
  * @note   原来 driver_ir_receiver.c 里的 IRReceiver_Test() 是 while(1) 死循环,
  *         这里改成非阻塞轮询 + osDelay, 才能和 FreeRTOS 共存
  */
void IRReceiverTask(void *argument)
{
  uint8_t dev, data;

  IRReceiver_Init();

  OLED_Clear();
  OLED_PrintString(0, 0, "IR Receiver:");
  OLED_PrintString(0, 2, "Dev    Data");

  for (;;)
  {
    if (IRReceiver_Read(&dev, &data) == 0)
    {
      OLED_PrintString(0, 4, "                ");
      OLED_PrintHex(0, 4, dev, 1);
      OLED_PrintHex(8, 4, data, 1);
      OLED_PrintString(0, 6, "                ");
      OLED_PrintString(0, 6, IRReceiver_CodeToString(data));
    }
    // osDelay(20);
  }
}

/**
  * @brief  普通 LED 任务: PC13 闪灯
  * @note   用 driver_led 的 API; 延时必须用 osDelay(让出 CPU),
  *         不能用 driver_timer 的 mdelay —— 那是忙等, 会把 CPU 占死
  */
void LedTask(void *argument)
{
  Led_Init();

  for (;;)
  {
    Led_Control(LED_GREEN, 1);   /* 亮 (PC13 低电平点亮) */
    osDelay(500);

    Led_Control(LED_GREEN, 0);   /* 灭 */
    osDelay(500);
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
  /* MyTask 里的 OLED_Test() 会不停刷满整屏, 与红外显示冲突, 暂时停用 */
  /* xTaskCreate(MyTask, "defaultTask2", 128, NULL, osPriorityNormal, NULL); */
  /* OLED 打印任务: 动态创建(栈从 FreeRTOS 堆里分配) */
  // xTaskCreate(OLEDPrintTask, "OLEDTask", 256, NULL, osPriorityNormal, NULL);

  /* 红外接收任务: 动态创建(栈从 FreeRTOS 堆里分配) */
  // xTaskCreate(IRReceiverTask, "IRTask", 256, NULL, osPriorityNormal, NULL);

  /* 普通 LED 任务: 静态创建(栈和 TCB 由用户提供, 不占 FreeRTOS 堆) */
  if (xTaskCreateStatic(LedTask,
                        "LedTask",
                        LED_TASK_STACK_SIZE,
                        NULL,
                        osPriorityNormal,
                        g_LedTaskStack,
                        &g_LedTaskTCB) == NULL)
  {
    Error_Handler();   /* 静态创建失败只可能是参数错误 */
  }
  /*使用同一个函数创建不同的任务*/
  /* 注意: 动态创建失败只返回 pdFAIL(NULL), 不会报错, 必须检查返回值 */
  if (xTaskCreate(OLEDPrintTask, "Task1", 128, &g_OLEDTaskParams1, osPriorityNormal, NULL) != pdPASS ||
      xTaskCreate(OLEDPrintTask, "Task2", 128, &g_OLEDTaskParams2, osPriorityNormal, NULL) != pdPASS ||
      xTaskCreate(OLEDPrintTask, "Task3", 128, &g_OLEDTaskParams3, osPriorityNormal, NULL) != pdPASS)
  {
    Error_Handler();   /* FreeRTOS 堆不够导致创建失败 */
  }
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
    HAL_Delay(100);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

