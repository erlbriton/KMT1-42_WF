#include "stm32f4xx.h"
#include "uart1toRS485.h"
#include "uart2toRS485.h"

#include "ramdata.h"
#include "flashdata.h"
#include "memutil.h"
#include "crc16.h"
#include "fram.h"

#include "rtc.h"
#include "DIO.h"
#include "stm32f4xx_i2c.h"
#include "intmash_dsp.h"

#define IWDG_1ms_TICKS 8
#define IWDG_KEY_RELOAD 0xAAAA
#define IWDG_KEY_START  0xCCCC
#define DISCR_FREQ 10000

ErrorStatus HSEStartUpStatus;
void  ADCs_Configuration(void); 
void  TIM8_Configuration(void); 

//--------------------------------------------------------------

void GPIO_Configuration(void) {
  GPIO_InitTypeDef GPIO_InitStructure; 
  
  // 1. ТАКТИРОВАНИЕ
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB | 
                         RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD, ENABLE); 
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;

  // 2. АНАЛОГОВЫЕ ВХОДЫ
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN; 
  GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_2 | GPIO_Pin_3;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  // 3. ЦИФРОВЫЕ ВЫХОДЫ (Open Drain)
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
//Порт А
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_15;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  //Порт С                       CS ADC
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  //---------------------------------- Входы------------------------------------------
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_4 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_11;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  // 4. ПЕРИФЕРИЯ (AF)
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; 

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_9 | GPIO_Pin_10;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource9,  GPIO_AF_USART1);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource2,  GPIO_AF_USART2);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource3,  GPIO_AF_USART2);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource5,  GPIO_AF_SPI1);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource6,  GPIO_AF_SPI1);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_12;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_SPI3);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource12, GPIO_AF_SPI3);

  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6 | GPIO_Pin_7;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_I2C1);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_I2C1);

  // 5. ЦИФРОВЫЕ ВХОДЫ
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; 

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8; // A8
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  //
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_9 | GPIO_Pin_10; 
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15; 
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; // D2
  GPIO_Init(GPIOD, &GPIO_InitStructure);

  // 6. ПРИВЯЗКА EXTI К ПОРТАМ
  SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource8);
  SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOB, EXTI_PinSource10);
}

//Таймер для работы с MODBUS клиентами
void TIM1_Configuration(void){
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  TIM_OCInitTypeDef TIM_OCInitStructure;
  
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);//включаем тактирование
   /* Time Base configuration */
  TIM_TimeBaseStructure.TIM_Prescaler = 168-1;//частота = TIM1CLK(168МГц)/168 = 1Мгц. один тик - микросекунда
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseStructure.TIM_Period = 0xffff;//
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;

  TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);
  TIM_ARRPreloadConfig(TIM1, ENABLE);
  
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Timing;       
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Disable;  
  TIM_OCInitStructure.TIM_Pulse = 0;
  TIM_OC1Init(TIM1, &TIM_OCInitStructure);//по каналу на каждый интерфейс УСАРТ (если их несколько)
  TIM_OC2Init(TIM1, &TIM_OCInitStructure);//U2

   /* TIM counter enable*/ 
  TIM_ITConfig(TIM1, TIM_IT_Update, DISABLE);//пока все прерывания отключены
  TIM_Cmd(TIM1, ENABLE);
  TIM1->SR = 0;
  //
}
//******************************************************************************
//Работа с аналоговыми сигналами: таймер для АЦП, настройка АЦП, настройка ДМА для передачи данных с АЦП 
void TIM8_Configuration(void) {
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  
  // Включаем тактирование TIM8 (Шина APB2 - 168 МГц)
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM8, ENABLE);

  /* Базовая конфигурация таймера */
  TIM_TimeBaseStructure.TIM_Prescaler = 0; // Частота тактирования 168 МГц
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  // Период: 168 000 000 / (16799 + 1) = 10 000 Гц (10 кГц)
  TIM_TimeBaseStructure.TIM_Period = 16799;
  TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;

  TIM_TimeBaseInit(TIM8, &TIM_TimeBaseStructure);  
  
  // Настройка внутреннего триггера: событие Update будет сигналом TRGO для АЦП
  TIM_SelectOutputTrigger(TIM8, TIM_TRGOSource_Update);

  // Запуск таймера
  TIM_Cmd(TIM8, ENABLE);  
}

volatile uint16_t spi_rx_buffer;
uint16_t spi_tx_dummy = 0x0000;

void ADCs_Configuration(void)
{
  DMA_InitTypeDef DMA_InitStructure; 

  // 1. Включаем тактирование периферии
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_SPI1, ENABLE);   

  // 2. Инициализация DMA для ВНУТРЕННЕГО АЦП (Stream 4)
  DMA_DeInit(DMA2_Stream4); 
  DMA_InitStructure.DMA_Channel = DMA_Channel_0;          
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &(ADC1->DR);
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t) &AIN_buf[0];
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
  DMA_InitStructure.DMA_BufferSize = 3;                   
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
  DMA_Init(DMA2_Stream4, &DMA_InitStructure);
  
  DMA_ITConfig(DMA2_Stream4, DMA_IT_TC, ENABLE);
  DMA_Cmd(DMA2_Stream4, ENABLE); 

  // 3. Инициализация DMA для ВНЕШНЕГО АЦП (SPI1 на Stream 0 и 3)
  // Настройка RX (Прием данных от TLC1549 в spi_rx_buffer)
  DMA_DeInit(DMA2_Stream0);
  DMA_InitStructure.DMA_Channel = DMA_Channel_3;          // Канал 3 для SPI1
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &(SPI1->DR);
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t) &spi_rx_buffer;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
  DMA_InitStructure.DMA_BufferSize = 1;                   // Читаем 1 полуслово (16 бит)
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Disable;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;           // Режим Normal для ручного перезапуска
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_Init(DMA2_Stream0, &DMA_InitStructure);

  // Настройка TX (Передача dummy в SPI1 из spi_tx_dummy)
  DMA_DeInit(DMA2_Stream3);
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t) &spi_tx_dummy;
  DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
  DMA_Init(DMA2_Stream3, &DMA_InitStructure);

  // Включаем запросы DMA со стороны SPI1
  SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx | SPI_I2S_DMAReq_Tx, ENABLE);

  // 4. Общая конфигурация ВНУТРЕННЕГО АЦП
  ADC_CommonInitTypeDef ADC_CommonInitStructure;
  ADC_CommonStructInit(&ADC_CommonInitStructure);
  ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
  ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4;
  ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
  ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_20Cycles;
  ADC_CommonInit(&ADC_CommonInitStructure);
  
  ADC_TempSensorVrefintCmd(ENABLE);

  // Конфигурация ADC1
  ADC_InitTypeDef ADC_InitStructure;
  ADC_StructInit(&ADC_InitStructure);  
  ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
  ADC_InitStructure.ADC_ScanConvMode = ENABLE; 
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
  ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_Rising;
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T8_TRGO;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfConversion = 3; 
  ADC_Init(ADC1, &ADC_InitStructure);  
  
  ADC_DMARequestAfterLastTransferCmd(ADC1, ENABLE);
  ADC_DMACmd(ADC1, ENABLE);  
  
  ADC_RegularChannelConfig(ADC1, ADC_Channel_12, 1, ADC_SampleTime_28Cycles); // Upwr_A
  ADC_RegularChannelConfig(ADC1, ADC_Channel_13, 2, ADC_SampleTime_28Cycles); // Upwr_B
  ADC_RegularChannelConfig(ADC1, ADC_Channel_17, 3, ADC_SampleTime_28Cycles); // Референс
}

//Таймер для dio частота 1Мгц
void TIM5_Configuration(void){
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);
   /* Time Base configuration */
  TIM_TimeBaseStructure.TIM_Prescaler = 84-1;//частота = TIM4CLK(84МГц)/84 = 1МГц.
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseStructure.TIM_Period = 1000;//частота переполнения  1кГц
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;

  TIM_TimeBaseInit(TIM5, &TIM_TimeBaseStructure);
  TIM_ARRPreloadConfig(TIM5, DISABLE);
  TIM5->SR = 0;
  TIM_ITConfig(TIM5, TIM_IT_Update, ENABLE);
  TIM_Cmd(TIM5, ENABLE);
  TIM5->SR = 0;
    
}
//Таймер общего назначения с прерыванием раз в 0,001 сек
void TIM6_Configuration(void){
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);
   /* Time Base configuration */
  TIM_TimeBaseStructure.TIM_Prescaler = 10-1;//частота = TIM4CLK(84МГц)/10 = 8,4МГц.
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseStructure.TIM_Period = 16800;//частота переполнения  1кГц
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;

  TIM_TimeBaseInit(TIM6, &TIM_TimeBaseStructure);
  TIM_ARRPreloadConfig(TIM6, DISABLE);
  TIM6->SR = 0;
  TIM_ITConfig(TIM6, TIM_IT_Update, ENABLE);
  TIM_Cmd(TIM6, ENABLE);
  TIM6->SR = 0;
    
}
//настройка SPI для работы с платой дисплея
//работа в режиме TX
void SPI3_Configuration(void){
  SPI_InitTypeDef  SPI_InitStructure;

  // SPI3 находится на шине APB1 (42 МГц)
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);

  /* SPI3 configuration */
  SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;

  // 42 МГц / 16 = 2.625 МГц
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16; 

  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial = 7;

  //используем SPI3
  SPI_Init(SPI3, &SPI_InitStructure);

  SPI_Cmd(SPI3, ENABLE);
}
//---------------настройка SPI для работы с АЦП выходного напряжения--------------------

void SPI1_Configuration(void) {
    SPI_InitTypeDef SPI_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

    /* Исправленная конфигурация для работы с DMA */
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex; // ОБЯЗАТЕЛЬНО для работы пары DMA TX/RX
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_16b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;

    // 84 МГц / 32 = 2.625 МГц (подходит для TLC1549)
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32; 
    
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;

    SPI_Init(SPI1, &SPI_InitStructure);
    SPI_Cmd(SPI1, ENABLE);
}
//Настраиваем I2C
void I2C1_Configuration(void) {
    GPIO_InitTypeDef  GPIO_InitStructure;
    I2C_InitTypeDef   I2C_InitStructure;

    // 1. Включаем тактирование только для модуля I2C1 (APB1)
    // Тактирование GPIOB уже включено в общей инициализации
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    // 2. Настройка альтернативных функций пинов
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_I2C1);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_I2C1);

    // Используем структуру, объявленную локально или глобально
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;       // Open Drain для I2C
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;     // Внешние подтяжки
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // 3. Сброс модуля I2C1
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C1, ENABLE);
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C1, DISABLE);

    // 4. Настройка параметров шины
    I2C_InitStructure.I2C_Mode                = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle           = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1         = 0x00;
    I2C_InitStructure.I2C_Ack                = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed          = 100000; 

    I2C_Init(I2C1, &I2C_InitStructure);
    I2C_Cmd(I2C1, ENABLE);
}
/*******************************************************************************
* Function Name  : Watchdog_configuration
* Description    : Настройка сторожевого таймера
* Input          : u16 reset_time_ms - время до перезагрузки в мс (от 1 до 511)
* Output         : None
* Return         : None
********************************************************************************/
void Watchdog_configuration(u16 reset_time_ms){
  RCC_LSICmd(ENABLE);
  while (!(RCC->CSR & RCC_CSR_LSIRDY)){};
  IWDG->KR = IWDG_WriteAccess_Enable;
  IWDG->RLR = reset_time_ms * IWDG_1ms_TICKS;
  IWDG->KR = IWDG_KEY_RELOAD;
  IWDG->KR = IWDG_KEY_START;
}
/*******************************************************************************
* Function Name  : NVIC_Configuration
* Description    : Configures Vector Table base location.
* Input          : None
* Output         : None
* Return         : None
********************************************************************************/
//разрешаем переходы по векторам и настраиваем преоритеты следующих прерываний:
void NVIC_Configuration(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;

#ifdef  VECT_TAB_RAM  
  /* Set the Vector Table base location at 0x20000000 */
  NVIC_SetVectorTable(NVIC_VectTab_RAM, 0x0); 
#else  /* VECT_TAB_FLASH  */
  /* Set the Vector Table base location at 0x08000000 */ 
  NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);   
#endif
  
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_3);
  
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  /* Enable TIM interrupts */
  NVIC_InitStructure.NVIC_IRQChannel = TIM1_CC_IRQn;//Таймер MOBBUS Slave
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 6;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;  
  NVIC_Init(&NVIC_InitStructure);
    
  NVIC_InitStructure.NVIC_IRQChannel = TIM6_DAC_IRQn;//таймер общего назначения
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 4;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_Init(&NVIC_InitStructure); 
  
  NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;//Modbus slave
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 6;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_Init(&NVIC_InitStructure); 
  
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;//Modbus slave
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 7;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_Init(&NVIC_InitStructure); 
    
  // Измерение тока и напряжения катушки (наивысший приоритет)
  NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream4_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; // Наивысший приоритет
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_Init(&NVIC_InitStructure);  
  
}

filter_struct Flt_Iomax;
filter_struct Flt_Io;

void Start_init (void){//начальная инициализация. 
  //разрешаем работу ДМА(для уартов)

  init_filter2_low(1.0f, 500.0f, DISCR_FREQ, &Flt_Iomax);//Фильтр 1-го порядка
  init_filter2_low(1.0f, 500.0f, DISCR_FREQ, &Flt_Io);//Фильтр 1-го порядка

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE); 
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);
  
  GPIO_Configuration();//конфигурируем порты
  TIM1_Configuration();//таймер для модбаса
  uart1to485_init();  //интерфейс для модбаса
  uart2to485_init();  //интерфейс для модбаса
  NVIC_Configuration();//конфигурируем прерывания
  
  //так же разрешаем работу системного таймера, для отладки
  //число для перезагрузки 0xffff для удобства расчета (16 бит)
  SysTick->LOAD  = 0xffff;      /* set reload register */  
  SysTick->VAL   = 0;           /* Load the SysTick Counter Value */
  SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;   
}

void check_flash (void)
{
  //функция проверяет целостность уставок во ФЛЕШ
  u8 *crcl1;
  u8 *crcl2;
  u8 *crch1;
  u8 *crch2;
  
  RAM_DATA.FLAGS.BA.backup_error = 0;
  RAM_DATA.FLAGS.BA.flash_error = 0;
  if (crc16((u8*)&FLASH_DATA, FlashTmpBufferSize))
  {    
    //контрольная сумма не сошлась, сектор битый или после прошивки
    if (crc16((u8*)&BKFLASH_DATA, FlashTmpBufferSize)) 
    {
      RAM_DATA.FLAGS.BA.flash_error = 1; //оба сектора битые или после прошивки
      RAM_DATA.FLAGS.BA.backup_error = 1; 
    }
    else  
    {
      FlashSectorWrite((u32)&FLASH_DATA, (u32)&BKFLASH_DATA);//пытаемся восстановить из бэкапа
      if (crc16((u8*)&FLASH_DATA, FlashTmpBufferSize)) RAM_DATA.FLAGS.BA.flash_error = 1; //не получилось, ставим ошибку
    }    
  }
  else
  {
     //нормальная ситуация, сектор в порядке
    if (crc16((u8*)&BKFLASH_DATA, FlashTmpBufferSize)) 
    {
      FlashSectorWrite((u32)&BKFLASH_DATA, (u32)&FLASH_DATA);//резервный сектор битый, пытаемся восстановить
      if (crc16((u8*)&BKFLASH_DATA, FlashTmpBufferSize)) RAM_DATA.FLAGS.BA.backup_error = 1; //не получилось       
    }
    else {//оба сектора в порядке
      crcl1 = (u8*)&FLASH_DATA + (FlashTmpBufferSize-2);
      crcl2 = (u8*)&BKFLASH_DATA + (FlashTmpBufferSize-2);
      crch1 = (u8*)&FLASH_DATA + (FlashTmpBufferSize-1);
      crch2 = (u8*)&BKFLASH_DATA + (FlashTmpBufferSize-1);
      if ((*crcl1 != *crcl2)||(*crch1 != *crch2)){//если контрольные суммы у секторов разные
        FlashSectorWrite((u32)&BKFLASH_DATA, (u32)&FLASH_DATA);//запишем в бекап основной сектор
      }
    }
  }
}

//инициализация периферии
void Init (void){ 
  u8 fram_status;
  u8 mem_status;
  u8 *crcl;
  u8 *crch;
  
  Start_init();//начальная инициализяция, достаточная для настройки устройства
  
  fram_status=check_fram();//проверяем ключ параметров
  if(fram_status & FRAM_ERROR) RAM_DATA.FLAGS.BA.fram_error=1;
  else RAM_DATA.FLAGS.BA.fram_error=0;
  if(fram_status & FRAM_BKP_ERROR) RAM_DATA.FLAGS.BA.fram_bkp_error=1;
  else RAM_DATA.FLAGS.BA.fram_bkp_error=0;
  
  check_flash();//проверяем флеш-сектора данных
  //по результатам проверки делаем необходимые восстановительные операции
  if (RAM_DATA.FLAGS.BA.flash_error) mem_status = 2;
  else mem_status = 0;
  if (RAM_DATA.FLAGS.BA.fram_error) mem_status++;
  
  switch (mem_status) {
    case 0x00: crcl = (u8*)&FLASH_DATA + (FlashTmpBufferSize-2);      
               crch = (u8*)&FLASH_DATA + (FlashTmpBufferSize-1);
               if ((*crcl != Fram_Buffer[FramTmpBufferSize-2])||(*crch != Fram_Buffer[FramTmpBufferSize-1])){//если контрольные суммы у секторов разные
                FlashSectorWrite((u32)&FLASH_DATA, (u32)&Fram_Buffer);//запишем в основной сектор
                FlashSectorWrite((u32)&BKFLASH_DATA, (u32)&Fram_Buffer);
                check_flash();
               }
               break;
    case 0x01: fram_wr_massive((u8*)&FLASH_DATA, FramTmpBufferSize, 0x00, fram_sector);
               fram_wr_massive((u8*)&FLASH_DATA, FramTmpBufferSize, 0x00, fram_bkp);
               fram_status=check_fram();//проверяем ключ параметров
               if(fram_status & FRAM_ERROR) RAM_DATA.FLAGS.BA.fram_error=1;
               else RAM_DATA.FLAGS.BA.fram_error=0;
               if(fram_status & FRAM_BKP_ERROR) RAM_DATA.FLAGS.BA.fram_bkp_error=1;
               else RAM_DATA.FLAGS.BA.fram_bkp_error=0;
               break;
    case 0x02: FlashSectorWrite((u32)&FLASH_DATA, (u32)&Fram_Buffer);
               FlashSectorWrite((u32)&BKFLASH_DATA, (u32)&Fram_Buffer);
               check_flash();
               break;
    default:   break;  
    
  }
  //провряем нет ли ошибок после восстановления, если есть - дальнейшая инициализация НЕ проводится.
  //работает только модбас через ЮСБ для просмотра состояния ошибки
  if (RAM_DATA.FLAGS.BA.flash_error) return;

  //все проверки прошли успешно, теперь можно инициализировать все остальное
  SPI3_Configuration();
  ADCs_Configuration(); //ацп+дма
  TIM6_Configuration(); //общего назначения     
  TIM8_Configuration(); //ацп      
}