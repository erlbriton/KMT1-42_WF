/* раньше преобразование выглядело так: 
fIin= (((float)AIN[I_IN_AIN])*REF_DEF/((float)AIN[REF_AIN]) - ((float)CD_DATA.0adc_Iin))*CD_DATA.Kadc_Iin * СD_DATA.Iin_Scope;

чтобы упростить запись, сделать удобнее, оптимизированнее:
1. в CD_DATA все калибровочные уставки в формате float; 
1. в сишнике редактируем AIOdataInit, AIOselfCAlibration (только для тестовой прошивки), редактируем макросы в хэдэре;
2. добавляем этот хэдэркуда надо (main, init, it или др.);
3. вызываем при инициализации (тожно так же после каждой записи в СD) AIOdataInit();
4. при каждой итерации прерывания по DMA вызывать макрос 5VREF_COMPENS; 
5. теперь то же преобразование будет иметь вид: fIin=(((float)AIN_buf[AIN_N])*K5VREF - OadcIin) * KadcIin;
6. Танцы!!!
7. преобразовываться
*/

#ifndef AIO_H
#define AIO_H

//девайны индексов каждого сигнала в буфере АЦП
#define Ua_in   0
#define Ub_in   1
#define Uc_in   2
#define Ua_out  3
#define Ub_out  4
#define Uc_out  5
#define I_a     6
#define I_b     7
#define I_c     8
#define I_load  9
#define U_load  10
#define U_Ref   11


//дефайны реверенсов
//#define REF_DEF   1500.0
//extern float REFIN;
//
//#define REF_COMPENS REFIN = REF_DEF/((float)AIN_buf[U_REF])

//калибровочные данные 
extern float Kadc_Uload;
extern float Kadc_Iload;
extern float Kadc_Iin;
extern float Kadc_Uin;
extern float Kadc_Udac1;
extern float Kadc_Udac2;
extern float Kadc_Uadc6;
extern float Kadc_Uadc7;

extern float Oadc_Uload;
extern float Oadc_Iload;
extern float Oadc_Iin;
extern float Oadc_Udac1;
extern float Oadc_Udac2;
extern float Oadc_Uadc6;
extern float Oadc_Uadc7;

//преобразованные данные (мгновенные)
extern float fUin_A;
extern float fUin_B;
extern float fUin_C;
extern float fUout_A;
extern float fUout_B;
extern float fUout_C;
extern float fIa;
extern float fIa;
extern float fIb;
extern float fIc;
extern float fUload;
extern float fIload;
  
//преобразованные данные (усредненные)
extern float fUin_A_Ave;
extern float fUin_B_Ave;
extern float fUin_C_Ave;
extern float fUout_A_Ave;
extern float fUout_B_Ave;
extern float fUout_C_Ave;
extern float fIa_Ave;
extern float fIb_Ave;
extern float fIc_Ave;
extern float fUload_Ave;
extern float fIload_Ave;




//функции расчета калибровочных данных
void AIOdataInit(void);
void AIOselfCAlibration(u16 SCcmd);

float RMS_sqrt(float);

#endif