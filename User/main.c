//#include "stm32f10x.h"                  // Device header
#include "string.h"
#include <stdio.h>

#include "bps_led.h"
#include "bps_usart.h"
#include "key.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"

/**************************** 全局变量 ********************************/

#define KEY1_EVENT (0x01 << 0)//设置事件掩码的位 0
#define KEY2_EVENT (0x01 << 1)//设置事件掩码的位 1
/**************************** 任务句柄 ********************************/
/* 
 * 任务句柄是一个指针，用于指向一个任务，当任务创建好之后，它就具有了一个任务句柄
 * 以后我们要想操作这个任务都需要通过这个任务句柄，如果是自身的任务操作自己，那么
 * 这个句柄可以为NULL。
 */
 /* 创建任务句柄 */
static TaskHandle_t AppTaskCreate_Handle = NULL;
//创建LED任务句柄
static TaskHandle_t  LED_Task_Handle=NULL;
//创建KEY任务句柄
static TaskHandle_t  KEY_Task_Handle = NULL;


//二值信号量句柄
EventGroupHandle_t Event_Handle=NULL;



//声明函数
static void KEY_Task(void* parameter);
static void LED_Task(void* parameter);


static void AppTaskCreate(void);

static void BSP_Init(void)
{
	/* 
	* STM32 中断优先级分组为 4，即 4bit 都用来表示抢占优先级，范围为：0~15 
	* 优先级分组只需要分组一次即可，以后如果有其他的任务需要用到中断， 
	* 都统一用这个优先级分组，千万不要再分组，切忌。 
	*/ 
	NVIC_PriorityGroupConfig( NVIC_PriorityGroup_4 ); 
	LED_GPIO_Config();
	KEY1_GPIO_Config();
	KEY2_GPIO_Config();
	USART_Config();
	
	//测试
//	led_G(on);
//	printf("串口测试");
}

int main()
{
	BaseType_t xReturn = NULL;/* 定义一个创建信息返回值，默认为pdPASS */
	
	
	
	BSP_Init();
	printf("这是全系列开发板-FreeRTOS-动态创建任务!\r\n");

	  /* 创建AppTaskCreate任务 */
  xReturn = xTaskCreate((TaskFunction_t )AppTaskCreate,  /* 任务入口函数 */
                        (const char*    )"AppTaskCreate",/* 任务名字 */
                        (uint16_t       )512,  /* 任务栈大小 */
                        (void*          )NULL,/* 任务入口函数参数 */
                        (UBaseType_t    )1, /* 任务的优先级 */
                        (TaskHandle_t*  )&AppTaskCreate_Handle);/* 任务控制块指针 */ 
																							
	if(xReturn==pdPASS)
	{
		printf("初始任务创建成功\r\n");
		vTaskStartScheduler();
	}
	else 
	{
		return -1;
	}
	while(1)
	{
		
	}

}


//KEY任务函数
static void KEY_Task(void* parameter)
{
	
	//EventBits_t xReturn = NULL;/* 定义一个创建信息返回值，默认为 pdTRUE */
	while(1)
	{
		if(key_scan(KEY1_GPIO_PORT,KEY1_GPIO_PIN)==1)
		{
			xEventGroupSetBits(Event_Handle,KEY1_EVENT);
			printf ( "KEY1被按下\n");
		}
		if(key_scan(KEY2_GPIO_PORT,KEY2_GPIO_PIN)==1)
		{
			xEventGroupSetBits(Event_Handle,KEY2_EVENT);
			printf ( "KEY2被按下\n");
		}
		vTaskDelay(20); 
	}
}

//LED任务函数
static void LED_Task(void* parameter)
{
	EventBits_t r_event; /* 定义一个事件接收变量 */ 
	while(1)
	{
		r_event = xEventGroupWaitBits(Event_Handle,
		                              KEY1_EVENT|KEY2_EVENT,/* 接收任务感兴趣的事件 */
		                              pdTRUE, /* pdTRUE：清除事件标志位  pdFALSE：不清除 */ 
		                              pdTRUE, /* pdTRUE：逻辑与   pdFALSE：逻辑或 */
		                              portMAX_DELAY);/* 指定超时事件,一直等 */
		if(r_event& (KEY1_EVENT|KEY2_EVENT)== (KEY1_EVENT|KEY2_EVENT) )
		{
			printf ( "KEY1 与 KEY2 都按下\n");
			LED_G_TOGGLE();
		}
		else
		{
			printf ( "事件错误\n");
		}
		
	}    
}




static void AppTaskCreate(void)
{
	BaseType_t xReturn = NULL;/* 定义一个创建信息返回值，默认为pdPASS */
	
	taskENTER_CRITICAL();           //进入临界区
	//创建一个事件
	Event_Handle=xEventGroupCreate();


	//创建低优先级任务函数
  xReturn=xTaskCreate((TaskFunction_t	)LED_Task,		//任务函数
															(const char* 	)"LED_Task",		//任务名称
															(uint16_t 		)512,	//任务堆栈大小
															(void* 		  	)NULL,				//传递给任务函数的参数
															(UBaseType_t 	)2, 	//任务优先级
															(TaskHandle_t*  )&LED_Task_Handle);/* 任务控制块指针 */ 	
															
	if(xReturn == pdPASS)/* 创建成功 */
		printf("LED_Task任务创建成功!\n");
	else
		printf("LED_Task任务创建失败!\n");
	
	
	 //创建中优先级任务
	 xReturn=xTaskCreate((TaskFunction_t	)KEY_Task,		//任务函数
															(const char* 	)"KEY_Task",		//任务名称
															(uint16_t 		)512,	//任务堆栈大小
															(void* 		  	)NULL,				//传递给任务函数的参数
															(UBaseType_t 	)3, 	//任务优先级
															(TaskHandle_t*  )&KEY_Task_Handle);/* 任务控制块指针 */ 
															
	if(xReturn == pdPASS)/* 创建成功 */
		printf("KEY_Task任务创建成功!\n");
	else
		printf("KEY_Task任务创建失败!\n");

	vTaskDelete(AppTaskCreate_Handle); //删除AppTaskCreate任务
	
	taskEXIT_CRITICAL();            //退出临界区
}


//静态创建任务才需要
///**
//  **********************************************************************
//  * @brief  获取空闲任务的任务堆栈和任务控制块内存
//	*					ppxTimerTaskTCBBuffer	:		任务控制块内存
//	*					ppxTimerTaskStackBuffer	:	任务堆栈内存
//	*					pulTimerTaskStackSize	:		任务堆栈大小
//  * @author  fire
//  * @version V1.0
//  * @date    2018-xx-xx
//  **********************************************************************
//  */ 
//void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, 
//								   StackType_t **ppxIdleTaskStackBuffer, 
//								   uint32_t *pulIdleTaskStackSize)
//{
//	*ppxIdleTaskTCBBuffer=&Idle_Task_TCB;/* 任务控制块内存 */
//	*ppxIdleTaskStackBuffer=Idle_Task_Stack;/* 任务堆栈内存 */
//	*pulIdleTaskStackSize=configMINIMAL_STACK_SIZE;/* 任务堆栈大小 */
//}



///**
//  *********************************************************************
//  * @brief  获取定时器任务的任务堆栈和任务控制块内存
//	*					ppxTimerTaskTCBBuffer	:		任务控制块内存
//	*					ppxTimerTaskStackBuffer	:	任务堆栈内存
//	*					pulTimerTaskStackSize	:		任务堆栈大小
//  * @author  fire
//  * @version V1.0
//  * @date    2018-xx-xx
//  **********************************************************************
//  */ 
//void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, 
//									StackType_t **ppxTimerTaskStackBuffer, 
//									uint32_t *pulTimerTaskStackSize)
//{
//	*ppxTimerTaskTCBBuffer=&Timer_Task_TCB;/* 任务控制块内存 */
//	*ppxTimerTaskStackBuffer=Timer_Task_Stack;/* 任务堆栈内存 */
//	*pulTimerTaskStackSize=configTIMER_TASK_STACK_DEPTH;/* 任务堆栈大小 */
//}
