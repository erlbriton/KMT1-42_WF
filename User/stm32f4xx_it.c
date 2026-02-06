/**
  ******************************************************************************
  * @file    Project/STM32F4xx_StdPeriph_Template/stm32f4xx_it.c 
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    30-September-2011
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and 
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_it.h"
#include "uart1toRS485.h"
#include "uart2toRS485.h"

#include "ramdata.h"
#include "flashdata.h"
#include "CDdata.h"


#include "IPCS_STM32F4.h"
#include "intmash_dsp.h"
#include "EFI_CPU.h"
#include "AIO.h"   
#include "DIO.h"

/** @addtogroup Template_Project
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

#define DMA_100HZ 100 //дефайн для усреднения переменных по 100Гц
#define DMA_50HZ 200 //дефайн для усреднения переменных периоду сети   
#define SCALE_DEC 10.0 //коэффициенты для разных шкал 0,1
#define SCALE_HUNDRED 100.0 // 0,01
#define SCALE_THOUSAND 1000.0  // 0,001
    
#define TIM_CNT_DMA 100 //число тиков таймера за 1 прерывание дма =100мкс = 100 тиков по 1 мкс
#define T_DMA 100 //число микросекунд за одно дма
#define MCSEC_TO_SEC 1000000 //преобразование микросекунд в секунды
#define ERROR_SYNC 125 //125 счетов - частота 40Гц, это уже плохо, ошибка синхры 105 - 48Гц
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
  
u8 RMS_Calc = 0; //флаг? по которому считать в мейне
u16 CNT_Uin_A = 0; //содержимое таймеров сифу фаз abc
//f32 Old_Uin_A = 0;//старые значения напряжения фаз для расчета частоты
f32 FreqA[5]; //массив для усреднения частоты
u8 Polar_sync = 0; //полуволна + или -

f32 New_Uin_A = 0;
f32 New_Uin_B = 0;

//сумматоры для действующих значений    
static float E_Uin_A;
static float E_Uin_B;
static float E_Uload = 0;

//Квадраты действующих значений

float E_Uin_A_global;
float E_Uin_B_global;

const float REF_DEF = 1500.0;
vf32 REFIN = 0;

static inline float RMS_sqrt(float x)
{
    return sqrt(x);
}

//модбас
void TIM1_CC_IRQHandler(void)
{  
  if ((TIM1->SR & TIM_IT_CC2)&&(TIM1->DIER & TIM_IT_CC2))//если прерывание вызвано паузой от USART2_to_RS485
  {
    TIM1->SR ^= TIM_IT_CC2; //снять флаг прерывания 
    U2_Timer();        
  }  
  
  if ((TIM1->SR & TIM_IT_CC1)&&(TIM1->DIER & TIM_IT_CC1))//если прерывание вызвано паузой от USART1_to_OPT
  {
    TIM1->SR ^= TIM_IT_CC1; //снять флаг прерывания 
    U1_Timer();        
  }  
  
}

//таймер дискретных входов выходов 
void TIM5_IRQHandler(void)
{ 
  TIM5->SR = 0;         
}

void TIM6_DAC_IRQHandler(void)
{  
  TIM6->SR ^= TIM_IT_Update;
  DIO_work();  
  RAM_DATA.DI_reg = RAM_DATA.DI_reg& 0x00ff; //если этого не сделать, то старший байт будет ff, потому что читаю только один байт
}

int a = 0;
void DMA2_Stream0_IRQHandler(void)//стрим ДМА, работающий с данными собственных аналоговых входов
{  
   DMA2->LIFCR = (uint32_t) (DMA_FLAG_FEIF0 | DMA_FLAG_DMEIF0 | DMA_FLAG_TEIF0 | DMA_FLAG_HTIF0 | DMA_FLAG_TCIF0);//почистим флаги стрима ДМА
  static u16 Cnt_100hz = 0; //счетчик для усреднения по 100 Гц 
  static u16 Cnt_50hz = 0; //счетчик для усреднения по периоду
  float fUin_A = 0;
  float fUin_B = 0;
  float fU_load = 0;
  
//=========================Измерение переменного  тока================================================= 
 
  New_Uin_A= filter2_Bandpass(&Filter_Uin_A, (float)AIN_buf[Ua_in]); //фильтрую, убираю постоянную,  теперь он +-
  New_Uin_B = filter2_Bandpass(&Filter_Uin_B,(float)AIN_buf[Ub_in]); //фильтрую, убираю постоянную,  теперь он +-
  
  //AIOdataInit();
  REFIN = REF_DEF/((float)AIN_buf[U_Ref]);
  
  fUin_A = New_Uin_A * REFIN * CD_DATA.K_Uin_A;
  fUin_B = New_Uin_B * REFIN * CD_DATA.K_Uin_B;

  E_Uin_A += fUin_A * fUin_A;
  E_Uin_B += fUin_B * fUin_B;
  
  Cnt_50hz ++;
  
  if(Cnt_50hz == DMA_50HZ)
  {
  E_Uin_A_global = E_Uin_A/DMA_50HZ;
  E_Uin_B_global = E_Uin_B/DMA_50HZ;

  RAM_DATA.Uin_A = RMS_sqrt(E_Uin_A_global);
  RAM_DATA.Uin_B = RMS_sqrt(E_Uin_B_global);
  RAM_DATA.U_REF = AIN_buf[U_Ref]; //оценка внутреннего референса
  
  //RMS_Calc = 1;//флаг, по которому считать рмс в мейне
  E_Uin_A = 0;
  E_Uin_B = 0;
  Cnt_50hz =0;
  }
  
  fU_load = ((float)AIN_buf[U_load] * REFIN - CD_DATA.O_Uload) * CD_DATA.K_Uload;//преобразовали в реальн вольты и нормировали по внутр рефу 
  E_Uload += fU_load;
  Cnt_100hz ++;
  if (Cnt_100hz == DMA_100HZ) //пришло время считать
  {
    RAM_DATA.Uload = (E_Uload)/DMA_100HZ;
    E_Uload = 0;
    Cnt_100hz = 0;
    
  }
}
/******************************************************************************/



void EXTI0_IRQHandler(void){  
  EXTI->PR = EXTI_Line0;
  
}


/******************************************************************************/

void EXTI9_5_IRQHandler(void){
  
  if (EXTI->PR & EXTI_Line7){
    EXTI->PR = EXTI_Line7;
   
  }
  if (EXTI->PR & EXTI_Line8){
    EXTI->PR = EXTI_Line8;
     }
  if (EXTI->PR & EXTI_Line9){
    EXTI->PR = EXTI_Line9;
  }
  
  
  
}




/******************************************************************************/
/*            Cortex-M4 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief   This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
  //TimingDelay_Decrement();
}

/******************************************************************************/
/*                 STM32F4xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f4xx.s).                                               */
/******************************************************************************/

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @}
  */ 


/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
