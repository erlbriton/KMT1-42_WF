#include "ramdata.h"
#include "CDdata.h"
#include "modbus_data.h"//содержит описания структур данных
#include "stm32f4xx_it.h"
#include "modbus.h"//тут есть функция декодирования принятого сообщения
#include "ramdata.h"
#include "memutil.h"

// static inline float RMS_sqrt(float x){
//     return sqrt(x);
// }

//#define SCALE_DEC 10.0 //коэффициенты для разных шкал 0,1
//#define SCALE_HUNDRED 100.0 // 0,01
//#define SCALE_THOUSAND 1000.0  // 0,001
//
//void CDdAtaWrite(void);

//float REFIN;

//float Kadc_Uload;
//float Kadc_Iload;
//float Kadc_Iin;
//float Kadc_Uin;
//float Kadc_Udac1;
//float Kadc_Udac2;
//float Kadc_Uadc6;
//float Kadc_Uadc7;
//
//float Oadc_Uload;
//float Oadc_Iload;
//float Oadc_Iin;
//float Oadc_Udac1;
//float Oadc_Udac2;
//float Oadc_Uadc6;
//float Oadc_Uadc7;
//
//float fUload;
//float fIload;
//float fIin;
//float fUin;
//float fUdac1;
//float fUdac2;
//float fUadc6;
//float fUadc7;
//
//float fUload_Ave;
//float fIload_Ave;
//float fIin_Ave;
//float fIomaxve;
//float fUdac1_Ave;
//float fUdac2_Ave;
//float fUadc6_Ave;
//float fUadc7_Ave;


//инициализация калибровочных данный на основе уставок из сектора CD_DATA
//void AIOdataInit(void)
//{
//  Kadc_Uin = CD_DATA.K_Uin;
//  Kadc_Uload = CD_DATA.K_Uload;
//  Kadc_Iload = CD_DATA.K_Iload;
//  Kadc_Iin = CD_DATA.K_Iin * CD_DATA.S_Iin;
//  Kadc_Udac1 = CD_DATA.K_DAC1 * CD_DATA.S_DAC1;
//  Kadc_Udac2 = CD_DATA.K_DAC2 * CD_DATA.S_DAC2;
//  Kadc_Uadc6 = CD_DATA.K_ADC6 * CD_DATA.S_ADC6;
//  Kadc_Uadc7 = CD_DATA.K_ADC7 * CD_DATA.S_ADC7;
//  
//  Oadc_Uload = CD_DATA.O_Uload;
//  Oadc_Iload = CD_DATA.O_Iload;
//  Oadc_Iin = CD_DATA.O_Iin;
//  Oadc_Udac1 = CD_DATA.O_DAC1;
//  Oadc_Udac2 = CD_DATA.O_DAC2;
//  Oadc_Uadc6 = CD_DATA.O_ADC6;
//  Oadc_Uadc7 = CD_DATA.O_ADC7;
//
//}
