#ifndef INTMASH_DSP_H
#define INTMASH_DSP_H

#include "arm_math.h"
// consts---------------------------------------
/*
	необходимые для расчетов табличные коэффициенты
	фильтра Баттерворта 2го порядка
*/
#define A1_KOEF 1.4142 //
#define B1_KOEF 1.0000 //
//----------------------------------------------------------------------------------------------------------------------------------
/*
для фильтрации используется передаточная характеристика, общей вид которой
		
 A(z) = Y(z) / X(z) = (D0 + D1*z + D2*z*z) / (C0 + C1*z + C2*z*z) = (D0*Z(-2) + D1*Z(-1) + D2) / (C0*Z(-2) + C1*Z(-1) + C2)
 для фильтр второго порядка С2 = 1, тогда
 Y(0) = (D0*X(-2) + D1*X(-1) + D2*X(0)) - (C0*Y(-2) + C1*Y(-1))

 для PSIM коэффициентов:
 a0= C2=1		 b0= D2
 a1= C1			 b1= D1
 a2= C0			 b2= D0

 для расчетов использовались формулы из книги Полупроводниковая схемотехника Титце, Шенк
*/


// структура коэффициентов для фильтров второго порядка
//для каждого фильтра создаем свою структура
typedef struct 
{
	float Cp[2];//коэффициенты знаменателя передаточной A(z)
	float Dp[2];//коэффициенты числителя передаточной A(z)
	float Xp[2]; //предыдущие значения х: X[0]=X-1, X[1]=X-2
	float Yp[2]; //предыдущие значения y: Y[0]=Y-1, Y[1]=Y-2
}filter_struct;

/*Name: init_filter2_low
	инициализация фильтра низких частот 2 порядка
Arg:
	A_gain - коэффициент усиления
	f_cutoff - частота среза
	f_sample - частота выборки
	*init_struct - указатель на структуру коэффициентов
Comment:
	в функции считаются коэффициенты для переходной функции A(z) и записываются в элементы структуры init_struct
	функция вызывается один раз (или при каждом изменении входных параметров)
*/
static inline void init_filter2_low(float A_gain, float f_cutoff, float f_sample, filter_struct *init_struct)
{
	float C0 = 0,C1 = 0, D_ = 0;
	float  Om = 0, l = 0;

	Om = f_sample/f_cutoff; //нормированная частота выборки
	l = tan(3.14 / Om);
	l = 1/l; // котангенс, коэффициент для билинейного преобразования P в Z (от аналоговой передаточной функции к цифровой)
	D_ = 1 + A1_KOEF*l + B1_KOEF*l*l; //знаменатель для всех формул коэффициентов
	C0 = 1 - A1_KOEF*l + B1_KOEF*l*l; //числитель С0
	init_struct->Cp[0] = C0 / D_;	//весь коэффициент С0
	C1 = 2 * (1 - B1_KOEF*l*l); //числитель С1
	init_struct->Cp[1] = C1 / D_; //весь коэффициент С1
	init_struct->Dp[0] = A_gain/D_; //весь коэффициент D0
}

/*Name: init_filter1_low
	инициализация фильтра низких частот 1 порядка
Arg:
	A_gain - коэффициент усиления
	f_cutoff - частота среза
	f_sample - частота выборки
	*init_struct - указатель на структуру коэффициентов
Comment:
	в функции считаются коэффициенты для переходной функции A(z) и записываются в элементы структуры init_struct
	функция вызывается один раз (или при каждом изменении входных параметров)
*/
static inline void init_filter1_low(float A_gain, float f_cutoff, float f_sample, filter_struct *init_struct)
{
	float C0 = 0, D_ = 0;
	float  TWc = 0;

	TWc = 2*3.14159*f_cutoff/f_sample; //коэффициент T*Wc= 1/fs * 2Pi* fc
	D_ = TWc/(TWc + 2); //коэффициент D0 и D1 без коэф усиления
	C0 = (TWc - 2)/(TWc + 2); //коэффициент С0
	init_struct->Cp[0] = C0;	
	init_struct->Dp[0] = A_gain * D_; //весь коэффициент D0
}

/*Name: init_filter2_high
	инициализация фильтра высоких частот 2 порядка
Arg:
	A_gain - коэффициент усиления
	f_cutoff - частота среза
	f_sample - частота выборки
	*init_struct - указатель на структуру коэффициентов
Comment:
	в функции считаются коэффициенты для переходной функции A(z) и записываются в элементы структуры init_struct
	функция вызывается один раз (или при каждом изменении входных параметров)
*/
static inline void init_filter2_high(float A_gain, float f_cutoff, float f_sample, filter_struct * init_struct)
{
	float C0 = 0,C1 = 0, D_ = 0;
	float  Om = 0, l = 0;

	Om = f_sample/f_cutoff;//нормированная частота выборки
	l = tan(3.14 / Om);
	l = 1/l; // котангенс, коэффициент для билинейного преобразования P в Z (от аналоговой передаточной функции к цифровой)
	D_ = B1_KOEF + A1_KOEF*l + l*l; //знаменатель для всех формул коэффициентов
	C0 = B1_KOEF - A1_KOEF*l + l*l; //числитель С0
	init_struct->Cp[0] = C0 / D_;	//весь коэффициент С0
	C1 = 2 * (B1_KOEF - l*l); //числитель
	init_struct->Cp[1] = C1 / D_; //весь коэффициент
	init_struct->Dp[0] = (A_gain * l * l)/D_; 
}

/*Name: init_filter2_bandpass
	инициализация фильтра полосового 2 порядка
Arg:
	A_gain - коэффициент усиления
	f_r - частота резонансная
	f_band - ширина полосы пропускания
	f_sample - частота выборки
	*init_struct - указатель на структуру коэффициентов
Comment:
	в функции считаются коэффициенты для переходной функции A(z) и записываются в элементы структуры init_struct
	функция вызывается один раз (или при каждом изменении входных параметров)
*/
static inline void init_filter2_bandpass(float A_gain, float f_r, float f_band, float f_sample, filter_struct * init_struct)
{
	float C0 = 0,C1 = 0, D_ = 0;
	float Om = 0, l = 0, Q = 0;

	Om = f_sample/f_r; //нормированная частота выборки
	l = tan(3.14 / Om);
	l = 1/l; // котангенс, коэффициент для билинейного преобразования P в Z (от аналоговой передаточной функции к цифровой)
	Q = f_r /f_band;// добротность
	D_ = 1 + l/Q + (l * l); //знаменатель для всех формул коэффициентов
	C0 = 1 - l/Q + (l * l); //числитель С0
	init_struct->Cp[0] = C0 / D_;	//весь коэффициент С0
	C1 = 2 * (1 - (l * l)); //числитель C1
	init_struct->Cp[1] = C1 / D_; //весь коэффициент C1
	init_struct->Dp[0] = (-(A_gain/Q) * l)/D_; //весь коэффициент D0

}

/*Name: init_filter2_bandstop
	инициализация фильтра режекторного 2 порядка
Arg:
	A_gain - коэффициент усиления
	f_r - частота резонансная
	f_band - ширина полосы заграждения
	f_sample - частота выборки
	*init_struct - указатель на структуру коэффициентов
Comment:
	в функции считаются коэффициенты для переходной функции A(z) и записываются в элементы структуры init_struct
	функция вызывается один раз (или при каждом изменении входных параметров)
*/
static inline void init_filter2_bandstop(float A_gain, float f_r, float f_band, float f_sample, filter_struct * init_struct)
{
	float C0 = 0,C1 = 0, D_ = 0;
	float Om = 0, l = 0, Q = 0;

	Om = f_sample/f_r; //нормированная частота выборки
	l = tan(3.14 / Om);
	l = 1/l; // котангенс, коэффициент для билинейного преобразования P в Z (от аналоговой передаточной функции к цифровой)
	Q = f_r /f_band; // добротность
	D_ = 1 + l/Q + (l * l); //знаменатель для всех формул коэффициентов
	C0 = 1 - l/Q + (l * l); //числитель С0
	init_struct->Cp[0] = C0 / D_;	//весь коэффициент С0
	C1 = 2 * (1 - (l * l)); //числитель C1
	init_struct->Cp[1] = C1 / D_; //весь коэффициент С1
	init_struct->Dp[0] = (A_gain *(1 + (l * l)))/D_; //весь коэффициент D0
	init_struct->Dp[1] = (2*A_gain *(1 - (l * l)))/D_; //весь коэффициент D1
}



/*Name: filter1_Low
	фильтр низких частот 1 порядка
Arg:
	*init_struct - указатель на структуру типа filter_struct
	in - входные данные
Возвращает фильтрованное значение
Comment:
	в функции считается уже  фильрованное значение, для рассчетов 
	коэффициенты для переходной функции A(z) берутся из элементов структуры init_struct
	функцию надо вызывать циклично с частотой дискретизации
*/
static inline float filter1_Low(filter_struct * init_struct, float in)
{
	float out = 0;//выходное фильтрованное значение
	out = init_struct->Dp[0]*(in + init_struct->Xp[0]) - init_struct->Cp[0]*init_struct->Yp[0];	
	init_struct->Xp[0] = in;//входное значение X(0) становится X(-1)
	init_struct->Yp[0] = out;//выходное значение Y(0) становится Y(-1)
	return out;
}

/*Name: filter2_Low
	фильтр низких частот 2 порядка
Arg:
	*init_struct - указатель на структуру типа filter_struct
	in - входные данные
Возвращает фильтрованное значение
Comment:
	в функции считается уже  фильрованное значение, для рассчетов 
	коэффициенты для переходной функции A(z) берутся из элементов структуры init_struct
	функцию надо вызывать циклично с частотой дискретизации
*/
static inline float filter2_Low(filter_struct * init_struct, float in)
{
	float out = 0;//выходное фильтрованное значение
	out = init_struct->Dp[0]*(in + 2* init_struct->Xp[0] + init_struct->Xp[1]) - init_struct->Cp[1]*init_struct->Yp[0] -init_struct->Cp[0]*init_struct->Yp[1];	

	init_struct->Xp[1] = init_struct->Xp[0];//значение X(-1) становится X(-2)
	init_struct->Xp[0] = in;//входное значение X(0) становится X(-1)
	init_struct->Yp[1] = init_struct->Yp[0];//значение Y(-1) становится Y(-2)
	init_struct->Yp[0] = out;//выходное значение Y(0) становится Y(-1)
	return out;
}

/*Name: filter2_High
	фильтр высоких частот 2 порядка
Arg:
	*init_struct - указатель на структуру типа filter_struct 
	in - входные данные
Возвращает фильтрованное значение
Comment:
	в функции считается уже  фильрованное значение, для рассчетов 
	коэффициенты для переходной функции A(z) берутся из элементов структуры init_struct
	функцию надо вызывать циклично с частотой дискретизации
*/
static inline float filter2_High(filter_struct * init_struct, float in)
{
	float out = 0;//выходное фильтрованное значение
	out = init_struct->Dp[0]*(init_struct->Xp[1] - 2* init_struct->Xp[0] +  in) - init_struct->Cp[1]*init_struct->Yp[0] -init_struct->Cp[0]*init_struct->Yp[1];	

	init_struct->Xp[1] = init_struct->Xp[0];//значение X(-1) становится X(-2)
	init_struct->Xp[0] = in;//входное значение X(0) становится X(-1)
	init_struct->Yp[1] = init_struct->Yp[0];//значение Y(-1) становится Y(-2)
	init_struct->Yp[0] = out;//выходное значение Y(0) становится Y(-1)
	return out;
}

/*Name: filter2_Bandpass
	фильтр полосовой 2 порядка
Arg:
	*init_struct - указатель на структуру типа filter_struct
	in - входные данные
Возвращает фильтрованное значение
Comment:
	в функции считается уже  фильрованное значение, для рассчетов 
	коэффициенты для переходной функции A(z) берутся из элементов структуры init_struct
	функцию надо вызывать циклично с частотой дискретизации
*/
static inline float filter2_Bandpass(filter_struct * init_struct, float in)
{
	float out = 0;//выходное фильтрованное значение

	out = init_struct->Dp[0]*(init_struct->Xp[1] - in) - init_struct->Cp[1]*init_struct->Yp[0] -init_struct->Cp[0]*init_struct->Yp[1];	
	init_struct->Xp[1] = init_struct->Xp[0];//значение X(-1) становится X(-2)
	init_struct->Xp[0] = in;//входное значение X(0) становится X(-1)
	init_struct->Yp[1] = init_struct->Yp[0];//значение Y(-1) становится Y(-2)
	init_struct->Yp[0] = out;//выходное значение Y(0) становится Y(-1)
	return out;
}

/*Name: filter2_Bandstop
	фильтр режекторный 2 порядка
Arg:
	*init_struct - указатель на структуру типа filter_struct
	in - входные данные
Возвращает фильтрованное значение
Comment:
	в функции считается уже  фильрованное значение, для рассчетов 
	коэффициенты для переходной функции A(z) берутся из элементов структуры init_struct
	функцию надо вызывать циклично с частотой дискретизации
*/
static inline float filter2_Bandstop(filter_struct * init_struct, float in)
{
	float out = 0;//выходное фильтрованное значение

	out = init_struct->Dp[1]*(init_struct->Xp[1]  + in) + init_struct->Dp[1]*init_struct->Xp[0] - init_struct->Cp[1]*init_struct->Yp[0] -init_struct->Cp[0]*init_struct->Yp[1];
	init_struct->Xp[1] = init_struct->Xp[0];//значение X(-1) становится X(-2)
	init_struct->Xp[0] = in;//входное значение X(0) становится X(-1)
	init_struct->Yp[1] = init_struct->Yp[0];//значение Y(-1) становится Y(-2)
	init_struct->Yp[0] = out;//выходное значение Y(0) становится Y(-1)
	return out;
}
/* Медианный фильтр. Простой и тупой********************************************
Накапливает данные, усредняет значение среди последних N точек.

Пример использования:
float UoutBuf[10];//создаем буфер данных для фильтра
MedianFilter_struct Ufilter;//создаем структуру фильтра
MedianFilterInit(&Ufilter, UoutBuf, 10);//инициализируем фильтр

//далее где-то вызываем функцию фильтрации
fU=MedianFilter(&Ufilter, fUnew);
*******************************************************************************/
typedef struct 
{
	float* DataBuffer;//указатель на буфер с точками
	float Sum;//накопитель для вычисления
	unsigned int Index;//индекс текущей точки в буфере
	unsigned int BufferSize;//количество точек усреднения
	
} MedianFilter_struct;


/*Name: MedianFilterInit
	Инициализация структуры медианного фильтра
Arg:
	MedianFilter_struct * init_struct; - структура фильтра, которую будем инициализировать
	float* Buffer; - указатель на буфер с точками	
	unsigned int BufferSize; - количество точек усреднения
Ret:
Comment:
	
*/
static inline void MedianFilterInit(MedianFilter_struct * init_struct, float* Buffer, unsigned int BufferSize)
{
	init_struct->DataBuffer = Buffer;// установили указатель
	init_struct->Sum=0.0;
	init_struct->Index=0;
	init_struct->BufferSize=BufferSize;
}

/*Name: MedianFilterValueInit
	Инициализация текущего значения выхода медианного фильтра
Arg:
	MedianFilter_struct * init_struct; - структура фильтра, которую будем инициализировать
	float Value; - значение, которым нужно инициализировать фильтр
	
Ret:
Comment:
	
*/
static inline void MedianFilterValueInit(MedianFilter_struct * Filter, float Value)
{	
	Filter->Sum=Value*((float)Filter->BufferSize);//инициализация сумматора
	for(unsigned int i=0; i<(Filter->BufferSize); i++){
		Filter->DataBuffer[i] = Value;
	}
	Filter->Index=0;
}

/*Name: MedianFilter
	Медианная фильтрация 
Arg:
	MedianFilter_struct * init_struct; - структура фильтра
	float in; - новая точка
Ret:
	фильтрованное значение (float)
Comment:
	
*/
static inline float MedianFilter(MedianFilter_struct* Filter, float in)
{	
    Filter->Sum -= Filter->DataBuffer[Filter->Index];
    Filter->Sum += in;
    Filter->DataBuffer[Filter->Index] = in;
    Filter->Index++;
    if(Filter->Index==Filter->BufferSize) Filter->Index=0;
    return (Filter->Sum/((float)Filter->BufferSize));     
}

/*Ниже раздел расчета корня для RMS */
/*
RMS считается через интеграл. Для этого надо вычислять квадратный корень

используются формулы для вычисления квадратного корня из числа для МК с плавающей точкой
и для МК без плавающей точки

функции взяты из http://chipenable.ru/index.php/programming-avr/item/144-sqrt-root.html
расчитаны на 16битную переменную

функции протестированы с PSIM 
IMD\projects\ST.SMFCB.STAT\simulation\PSIM\выбор вычисления корня
*/

/*Name: RMS_root1
	приблизительное значение квадратного корня числа Х. работает с целыми числами величиной 4 байта
Arg:
	x - число, из которого надо получить корень квадратный
Comment:
	
*/

static inline unsigned long int isqrt32(unsigned long int x)
{
   unsigned long int m, y, b;
   m = 0x40000000;
   y = 0;
   while (m != 0){
      b = y | m;
      y = y >> 1;
      if (x >= b) {
         x = x - b;
         y = y | m;
      }
      m = m >> 2;
   }
   return y;
}

/*
работает с числами величиной  2 байта
чем больше итераций x = (a/x + x)>>1;
в функции, тем она точнее
*/
static inline unsigned int RMS_root2(unsigned int a) 
{
   unsigned int x;
   x = (a/0x3f + 0x3f)>>1;
   x = (a/x + x)>>1;
   x = (a/x + x)>>1;
   return(x); 
}

/*Name: RMS_sqrt
	вычисление квадратного корня, числа с плавающей точкой
Arg:
	x - число, из которого надо получить корень квадратный
Возвращает квадратный корень, число с плавающей точкой
Comment:

*/
static inline float RMS_sqrt(float x)
{
    return sqrt(x);
}

/* Ниже раздел регулятора (ПИ, ПИД)******************************************/
/* 
Формула регулятора следующая:
  out = P_term + I_term + D_term, 
рассмотрим каждую часть поподробней:

P_term - пропорциональная часть. тут все просто, P_term = pGain*error,
где error = задание - значение регулируемой величины;

I_term - тут тоже все относительно просто: I_term = iGain*sum(error),
где sum(error) - сумма всех ошибок за все время регулирования, 
сумма ограничивается сверху и снизу, чтобы не уйти в насыщение и 
не допустить переполнения соответствующей переменной.

D_term - вот тут уже не все так просто. в данной версии регулятора 
дифференциальная часть может быть расчитана тремя способами:

dif_type = 0:
  D_term = 0, будет просто ПИ регулятор;

в остальных способах D_term = dGain*(position - last_position), где
dif_type = 1:
  position, last_position - значение error в текущий и в прошлый моменты дискретезации 
  этот тип расчета предпочтительней, если задание постоянно меняется и 
  необходимо предугадывать изменение ошибки;
dif_type = 2:
  position, last_position - значениевходного сигнала в текущий и 
  в прошлый моменты дискретезации этот тип расчета предпочтительней, 
  если необходима мягкость управления при резкой смене целевого состояния системы.
  в этом случает dGain берется со знаком МИНУС.*/

#define DTYPE_PI_ONLY           0
#define DTYPE_PID_CLASSIC       1
#define DTYPE_PID_SOFT          2

/*
Далее идет описание структуры регулятора, функции инициализации этой структуры 
(если необходимо не вручную задать все коэфициенты и ограничения, а расчитать),
ну и сама функция регулятора.
*/

typedef struct{
  float pGain, iGain, dGain; //коэффициенты регулятора
  float iState; //накопитель интегральной части
  float last_position; //прошлое состояние системы, для расчета диф. части
  float iMax, iMin; //ограничители интегральной части
  float outMax, outMin; //ограничители выхода регулятора  
} PID_f32_struct;

/*Name: PID_f32_init
	инициализация регулятора
Arg:
        *PID - указатель на структуру регулятора
	Gain - коэффициент усиления
	Ti - постоянная времени интегрирования (в секундах)
        Td - постоянная времени дифференцирования (в секундах)
	Ts - время семплирования (период дискретизации) (в секундах)       
        PIDmax, PIDmin - ограничения выходного значения регулятора
        PIDstart - стартовое значение выхода регулятора	
        dif_type - тип расчета дифференциальной части регулятора:
            0 - ПИ регулятор (диф. часть равна нулю)
            1 - расчет по дифференциалу ошибки (классический вариант)
            2 - расчет по дифференциалу входного сигнала
Comment:
	в функции расчитываются элементы структуры регулятора
	функция вызывается один раз (или при каждом изменении входных параметров)
*/
static inline void PID_f32_init(PID_f32_struct* PID, float Gain, float Ti, float Td, float Ts, float PIDmax, float PIDmin, float PIDstart, unsigned int dif_type){
  
  PID->pGain = Gain;// определяем пропорциональный коэффициент
  PID->iGain = (Gain * Ts)/Ti;// определяем интегральный коэффициент
  if(dif_type){//смотрим тип регулятора
    PID->dGain = (Gain * Td)/Ts;// определяем дифференциальный коэффициент
    // если выбран режим дифференцирования по , берем коэффициент со знаком минус
    if(dif_type==DTYPE_PID_SOFT) PID->dGain = (PID->dGain)*(-1);     
  }
  else PID->dGain = 0;//ПИ-регулятора
  //определяем ограничения регулятора и стартовое значение интегратора
  PID->outMax = PIDmax;
  PID->outMin = PIDmin;
  PID->iMax = PIDmax/(PID->iGain);
  PID->iMin = PIDmin/(PID->iGain);
  PID->iState = PIDstart/(PID->iGain);
  
}

/*Name: PID_f32
	ПИ/ПИД регулятора
Arg:
	*PID - указатель на структуру регулятора;
	error - ошибка (разница между заданием и входным сигналом);
        position - текущее состояние системы (для расчета диф. части):
          если DTYPE_PI_ONLY - position игнориуется,
          если DTYPE_PID_CLASSIC -  position ДОЛЖЕН быть равен error;
          если DTYPE_PID_SOFT - position равен входному сигналу, по которому ведется регулирование

Возвращает ограниченное значение ПИД/ПИ регулятора.
Comment:
	Необходимо, чтобы были заданы ВСЕ элементы структуры регулятора
*/

static inline float PID_f32(PID_f32_struct* PID, float error, float position){
  float PIDout;//временная переменная для хранения выходного значение
  
  PID->iState += error;//складываем интегратор новое значение ошибки
  //ограничиваем сверху и снизу:
  if((PID->iState)>(PID->iMax)) PID->iState = PID->iMax;
  else if ((PID->iState)<(PID->iMin)) PID->iState = PID->iMin;
  //расчет выходного значения регулятора:
  PIDout = (PID->pGain)*(error) + (PID->iGain)*(PID->iState) + (PID->dGain)*(position - (PID->last_position));
  //ограничиваем выходное значение сверху и снизу
  if (PIDout > (PID->outMax)) return (PID->outMax);
  else if (PIDout < (PID->outMin)) return (PID->outMin);
  return PIDout;
}


/***кусок резонансного регулятора, потом его встроить в дсп интмаш***/
typedef struct 
{
  float ResReg_B0;//коэффицент из стандартной формулы резонансного регулятора - пропорциональный
  float ResReg_A1;//коэффицент из стандартной формулы резонансного регулятора
  float outMax, outMin; //ограничители выхода регулятора
  float outHalf; //половина выходного значения сигнала
  float Xp[3]; //предыдущие значения х: X[0]=X-1, X[1]=X-2
  float Yp[3]; //предыдущие значения y: Y[0]=Y-1, Y[1]=Y-2
}ResonansReg_struct ; 

/*Name: ResonansReg_init
	расчет коэффициентов для резонансного регулятора
Arg:
	ResonansReg_struct * RegStr - указатель на структуру резонансного регулятора
	Ts - время сэмплирования,период дискретизации сигнала (в секуднах)
	F_sign - частота сигнала
        REGmax, REGmin - ограничения выходного значения регулятора
        REGhalf - половина максимального значения регулируемого параметра (используется для синусоидальных сигналов)
Comment:
	в функции считаются коэффициенты для функции H(z) и записываются в элементы структуры 
	функция вызывается один раз (или при каждом изменении входных параметров)
*/
static inline void ResonansReg_init(ResonansReg_struct * RegStr, float Ts, float F_sign, float REGmax, float REGmin, float REGhalf, float Gain)
{
  RegStr->outMax = REGmax;
  RegStr->outMin = REGmin;
  RegStr->ResReg_B0 = (Gain *Ts)/(1 + PI*PI*Ts*Ts*F_sign*F_sign);
  RegStr->ResReg_A1 = 2*(PI*PI*Ts*Ts*F_sign*F_sign - 1)/(PI*PI*Ts*Ts*F_sign*F_sign + 1);
  RegStr->outHalf = REGhalf;
}
  
/*Name: Resonans_reg
	расчет коэффициентов для резонансного регулятора
Arg:
	ResonansReg_struct * RegStr - указатель на структуру резонансного регулятора
	Data_in - новое значение регулируемого параметра
Comment:
	в функции происходит расчет для
*/
static inline float Resonans_reg(ResonansReg_struct * RegStr, float Data_in)
{
  float out = 0;
 
  RegStr->Xp[2] = RegStr->Xp[1];//значение X(-1) становится X(-2)
  RegStr->Xp[1] = RegStr->Xp[0];//значение X(0) становится X(-1)
  RegStr->Xp[0] = Data_in;

  RegStr->Yp[2] = RegStr->Yp[1];//значение Y(-1) становится Y(-2)
  RegStr->Yp[1] = RegStr->Yp[0];//значение Y(0) становится Y(-1)
  RegStr->Yp[0] = RegStr->ResReg_B0* (RegStr->Xp[0] - RegStr->Xp[2])  - RegStr->ResReg_A1 *RegStr->Yp[1] - RegStr->Yp[2];

 out = RegStr->Yp[0] + RegStr->outHalf; 
  if (out < RegStr->outMin) out = RegStr->outMin;
  if (out > RegStr->outMax) out = RegStr->outMax;

  return out;
}
/***проверить и вставить в дсп**/
#endif // INTMASH_DSP_H
