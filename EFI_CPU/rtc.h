#include "stm32f4xx.h"


    typedef struct
    {
        uint8_t Year;      // Год
        uint8_t Month;     // Месяц
        uint8_t Day;       // День месяца
        
        uint8_t DayOfWeek; // День недели
        
        uint8_t Hours;     // Часы
        uint8_t Minutes;   // Минуты
        uint8_t Seconds;   // Секунды
        uint8_t Options;   // на всякий случай
    } TDateTime;

// Инициализация модуля
void RTC_Configuration(void);

// Сброс состояния часов
void rtc_Reset(void);
    
// Получить текущее время
void rtc_Get(TDateTime * DateTime);
    
void rtc_SetDate(TDateTime * DateTime);
void rtc_SetTime(TDateTime * DateTime);

void rtc_Unlock(void);
void rtc_Lock(void);
    
