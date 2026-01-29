#include "rtc.h"


#define UNLOCK_RTC_1 0xCA //первое значение для записи для разблокировки
#define UNLOCK_RTC_2 0x53 //второе значение для записи для разблокировки
#define LOCK_RTC 0xFF //любая чушь, отличная от предыдущих


// Выключить защиту от записи
void rtc_Unlock(void)
{
    // Запишем эти значения по очереди
    RTC->WPR = UNLOCK_RTC_1;
    RTC->WPR = UNLOCK_RTC_2;    
    // Войдём в режим инициализации:
    RTC->ISR |= RTC_ISR_INIT;       
    // Ждём, когда это произойдёт
    while(!(RTC->ISR & RTC_ISR_INITF)) {}
    // теперь Здесь можем менять регистры RTC
}

// Включить защиту от записи
void rtc_Lock(void)
{
  // Инициализация закончилась
   RTC->ISR &= ~RTC_ISR_INIT;
    // Запишем какую-нибудь фигню, главное, чтоб не правильную
    RTC->WPR = LOCK_RTC;
}

// Установить дату
void rtc_SetDate(TDateTime * DateTime) //static
{
          rtc_Unlock();
    uint32_t Tens, Units;
    uint32_t TempReg = 0;   
    // Очистим поле даты
    TempReg = 0;    
    // Запишем год
        Tens  = (DateTime->Year / 10) & 0x0f;          // Десятки лет
        Units = (DateTime->Year - (Tens * 10)) & 0x0f; // Единицы лет       
        TempReg |= (Tens  << 20); // YT, 20
        TempReg |= (Units << 16); // YU, 16
    // Запишем месяц
        Tens  = (DateTime->Month / 10) & 0x01;          // Десятки месяцев
        Units = (DateTime->Month - (Tens * 10)) & 0x0f; // Единицы месяцев
        TempReg |= (Tens  << 12); // MT, 12
        TempReg |= (Units << 8);  // MU, 8
    // Запишем день
        Tens  = (DateTime->Day / 10) & 0x03;          // Десятки дней
        Units = (DateTime->Day - (Tens * 10)) & 0x0f; // Единицы дней        
        TempReg |= (Tens  << 4); // DT, 4
        TempReg |= (Units << 0);  // DU, 0
    // День недели:
        TempReg |= ((DateTime->DayOfWeek & 0x07) << 13); // WDU, 13   
    // Записывать надо всё сразу

    RTC->DR = TempReg;
    rtc_Lock();
}

// Установить время
 void rtc_SetTime(TDateTime * DateTime) //static
{
    rtc_Unlock();
    uint32_t Tens, Units;
    uint32_t TempReg = 0;   
    // Очистим поле даты
    TempReg = 0;    
    // Запишем часы
        Tens  = (DateTime->Hours / 10) & 0x03;          // Десятки часов
        Units = (DateTime->Hours - (Tens * 10)) & 0x0f; // Единицы часов        
        TempReg |= (Tens  << 20); // HT, 20
        TempReg |= (Units << 16); // HU, 16
    // Запишем минуты
        Tens  = (DateTime->Minutes / 10) & 0x07;          // Десятки минут
        Units = (DateTime->Minutes - (Tens * 10)) & 0x0f; // Единицы минут        
        TempReg |= (Tens  << 12); // MNT, 12
        TempReg |= (Units << 8);  // MNU, 8
    // Запишем секунды
        Tens  = (DateTime->Seconds / 10) & 0x07;          // Десятки секунд
        Units = (DateTime->Seconds - (Tens * 10)) & 0x0f; // Единицы секунд      
        TempReg |= (Tens  << 4); // ST, 4
        TempReg |= (Units << 0);  // SU, 0   
    // Записывать надо всё сразу    
    RTC->TR = TempReg;
    rtc_Lock();
}

// Сброс состояния часов
void rtc_Reset(void)
{
    // Включим тактирование PWR
    RCC->APB1ENR |= RCC_APB1ENR_PWREN; 
    // Разрешим доступ к управляющим регистрам энергонезависимого домена
    PWR->CR |= PWR_CR_DBP;   
    // Выберем его как источник тактирования RTC:
    RCC->BDCR |=  RCC_BDCR_BDRST;
    RCC->BDCR &= ~RCC_BDCR_BDRST;
}

// Получить текущее время
void rtc_Get(TDateTime * DateTime)
{
    uint32_t Date = RTC->DR;
    uint32_t Time = RTC->TR;   
    // Год
    DateTime->Year      = ((Date >> 20) & 0x0f) * 10 + ((Date >> 16) & 0x0f);
    // Месяц
    DateTime->Month     = ((Date >> 12) & 0x01) * 10 + ((Date >>  8) & 0x0f);
    // День
    DateTime->Day       = ((Date >>  4) & 0x03) * 10 + ((Date >>  0) & 0x0f);
    // День недели
    DateTime->DayOfWeek = ((Date >> 13) & 0x07);   
    // Час
    DateTime->Hours     = ((Time >> 20) & 0x03) * 10 + ((Time >> 16) & 0x0f);
    // Минуты
    DateTime->Minutes   = ((Time >> 12) & 0x07) * 10 + ((Time >> 8) & 0x0f);
    // Секунды
    DateTime->Seconds   = ((Time >> 4) & 0x07) * 10 + ((Time >> 0) & 0x0f);
}
//инициализация 
void RTC_Configuration(void)
{
  // Если часы запущены, делать тут нечего.  
  if(RTC->ISR & RTC_ISR_INITS)  { return;}
   /* Enable the PWR clock */
   RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);  
   RTC_DeInit();
   
   // Разрешим доступ к управляющим регистрам энергонезависимого домена
    // Allow access to RTC /
  PWR_BackupAccessCmd(ENABLE); 
  // Enable the LSE OSC
  RCC_LSEConfig(RCC_LSE_ON);
  // Wait till LSE is ready
  while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)  {}
  // Select the RTC Clock Source
  RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
  
  // Enable the RTC Clock /
  RCC_RTCCLKCmd(ENABLE);  
  // Wait for RTC APB registers synchronisation (needed after start-up from Reset)
    RTC_WaitForSynchro();
    
    //отключили защиту регистров и вкл инициализацию
    rtc_Unlock();
    // Часы остановлены. Режим инициализации
        // Настроим предделитель для получения частоты 1 Гц.       
        // LSI: 
        // LSE: нужно разделить на 0x7fff (кварцы так точно рассчитаны на это)
        {
            uint32_t Sync = 0xff;   // 15 бит
            uint32_t Async = 127;  // 7 бит            
            // Сначала записываем величину для синхронного предделителя
            RTC->PRER = Sync;           
            // Теперь добавим для асинхронного предделителя
            RTC->PRER = Sync | (Async << 16);
        }  
  // Инициализация закончилась
    rtc_Lock();
}

