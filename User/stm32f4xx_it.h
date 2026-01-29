/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32F4xx_IT_H
#define __STM32F4xx_IT_H

#ifdef __cplusplus
 extern "C" {
#endif 

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"

   
  extern float E_Ain_global;//сумматоры для действующих значений для вычисления рмс в мейне
  extern float E_Bin_global;
  extern float E_Cin_global;  
  extern float E_Aout_global;
  extern float E_Bout_global;
  extern float E_Cout_global;
  extern float E_Ia_global;
  extern float E_Ib_global;
  extern float E_Ic_global;
  extern u8 RMS_Calc;
  
  extern float E_Uin_global;
  extern float E_Uload_global;
  
   
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __STM32F4xx_IT_H */

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
