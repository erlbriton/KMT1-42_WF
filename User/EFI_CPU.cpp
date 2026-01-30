#include "stm32f4xx.h"
#include "ramdata.h"
#include "flashdata.h"
#include "CDdata.h"
#include "intmash_dsp.h" 
#include "memutil.h"
#include "crc16.h"
#include "mbtypes.h"
#include "AIO.h"
#include "EFI_CPU.h"

//параметры фильтра
#define F_again 1 //коэффициент усилени€
#define F_f_res 500 //резонансна€ частота
#define F_f_band 30 //ширина полосы пропускани€
#define F_f_sample 10000 //частота выборки 10к√ц

filter_struct Filter_Iomax; //структура параметров дл€ полосового фильтра фаза ј вход
filter_struct Filter_Io; //структура параметров дл€ полосового фильтра фаза B вход

u8 DI_stat = 0; //в кем из вх вых работаем в данный момент
u8 DO_stat = 0;

void EFICPUdataInit(void)
{  
    //инициализаци€ полосового фильтра
  // init_filter1_low(F_again, F_f_res, F_f_band, F_f_sample, &Filter_Iomax);
  // init_filter1_low(F_again, F_f_res, F_f_band, F_f_sample, &Filter_Io);
  init_filter1_low(F_again, F_f_res,  F_f_sample, &Filter_Iomax);
  init_filter1_low(F_again, F_f_res,  F_f_sample, &Filter_Io);

}
