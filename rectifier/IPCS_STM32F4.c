#include "stm32f4xx.h"
#include "ramdata.h"
#include "flashdata.h"


#define FLAG_CCR1 TIM_IT_CC1
#define FLAG_CCR2 TIM_IT_CC2
#define FLAG_UPD  TIM_IT_Update


//фазы и пол€рности


//сообщаем фазу и пол€рность и включаем или выключаем
/*
void ThyristorOFF(u8 Phase_Pol){

  switch (Phase_Pol){
  case TIM_A_plus:
    {
      TIM3->CCR1 = 0; //ноль шимить
    }
    break;
  case TIM_A_minus:
    {
      TIM3->CCR2 = 0; //ноль шимить
    }
    break;
  }

}*/

//30% шимим 
/*
void ThyristorON(u8 Phase_Pol){   
      
    switch (Phase_Pol){
  case TIM_A_plus:
    {
      TIM3->CCR1 = FLASH_DATA.TirPWM_Pulse;// 126; //сколько шимить
    }
    break;
  case TIM_A_minus:
    {
      TIM3->CCR2 = FLASH_DATA.TirPWM_Pulse;// 126; //сколько шимить
    }
    break;
  }

}*/

//шим ключом тиристоров
void TIM3_IRQHandler(void)
{
   TIM3->SR = 0;
}



/******************************************************************************/
void TIM2_IRQHandler(void){ 
  
    if (TIM2->SR & TIM_IT_CC1)
   {
     TIM2->SR = 0;     
    // if(Pwm_A_disable == 0)ThyristorON(TIM_A_plus); //отсчиталс€ угол, включаем шим - импульс управлени€ тиристором
   } 
  if (TIM2->SR & TIM_IT_CC2)
   { 
     TIM2->SR = 0;
    // ThyristorOFF(TIM_A_plus); //импульс кончилс€, выкл шим     
   }
    if (TIM2->SR & TIM_IT_CC3)
   {
     TIM2->SR = 0;      
    // if(Pwm_A_disable == 0)ThyristorON(TIM_A_minus); 
   } 
   if (TIM2->SR & TIM_IT_CC4)
   { 
     TIM2->SR = 0;
    // ThyristorOFF(TIM_A_minus);   
   }
    
}

