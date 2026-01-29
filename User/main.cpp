/**
  ******************************************************************************
  * @file    
  * @author  Vasiltsov
  * @version V0.0.2
  * @date    22/01/2026
  * @brief   Main program body.
  ******************************************************************************
  
  */ 

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "ramdata.h"
#include "flashdata.h"
#include "CDdata.h"
#include "rtc.h"
extern "C" {
#include "uart1toRS485.h"//
#include "uart2toRS485.h"//
#include "init.h"//
#include "id.h"//
#include "EFI_CPU.h"//
}
#include "intmash_dsp.h" 
#include "IPCS_STM32F4.h"
#include "stm32f4xx_it.h"
//#include "intmash_RMS.h"

/* defines ------------------------------------------------------------------*/


#define IPCS_RESET      alfa_IPCS[0]=0xffff    
/* Private function prototypes -----------------------------------------------*/
void  data_init(void);   
/* Private variables ---------------------------------------------------------*/  

char* IDtext  ="KMT1-42 ver.2.0.0 22.01.2026 www.intmash.ru";
/**
  * @brief  Main program.
  
  */
int main(void) 
{  

  static u16 link_tmp = 0;
  IDinit(CD_DATA.serial_number, IDtext);    
  Init();

  EFICPUdataInit(); //инициализируем все фильтры

  if (RAM_DATA.FLAGS.BA.flash_error)//если флешка бита€, либо после инициализации
  {  
      while (1)
      {
        if ((U2_SwCNT())||(U1_SwCNT()))
        {        
          if (LED_LINK_ST) LED_LINK_OFF;
          else LED_LINK_ON;
         link_tmp = 0;
        }  
        else
        { 
          if(link_tmp == 10000)
            LED_LINK_OFF;
          else link_tmp ++;
        }
      }      
  }
    //инициализаци€ параметров перед запуском
 
//------------------------------------------------------------------------------
  while (1)
  { 
//--------------»ндикаци€ работы интерфейсов св€зи----------------------------------------------------
    if((U2_SwCNT())||(U1_SwCNT()))//≈сли прин€т пакет 1-м или 2-м RS-485
    {       
      if (LED_LINK_ST) 
        LED_LINK_OFF;
      else LED_LINK_ON;
      link_tmp = 0;
    }  
    else
    { 
      if(link_tmp == 10000) 
        LED_LINK_OFF;
      else
        link_tmp ++;
    }  
    
////    //цикл тестовой прошивки
////    if(RAM_DATA.FLAGS.BA.LED_RED)LED_ALRM_ON; //дл€ тестовой прошивки проверить светодиод. остальные мигают сами
////    else LED_ALRM_OFF;

   // EFICPUprocess();
   
//    TIM3->CCR1 = RAM_DATA.Wpwm1; 
//    TIM3->CCR2 = RAM_DATA.Wpwm2;

//    if(RMS_Calc == 1) //считаетм рмс
//    {
//      RAM_DATA.Uin = RMS_sqrt(E_Uin_global);
//      RAM_DATA.Uload = RMS_sqrt(E_Uload_global);
//    }
    
  }  
}













#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif


/******************* (C) COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/
