//modbus rtu интерфейс организующий доступ к параметрам процессора через
//USART to USB интерфейс
//Используется:
//UART3 (RX, TX, )
//Timer1 (канал сравнения 3) (отсчёт временных интервалов Modbus RTU)

#include "flashdata.h"//тут у нас хранятся уставки для настройки интерфейса
#include "CDdata.h"//тут у нас хранятся уставки для настройки интерфейса
#include "modbus.h"//тут есть функция декодирования принятого сообщения


TClient uart3data;
void Rx3DMA (void);//настройка DMA на чтение данных из UART3
void Tx3DMA (void);//настройка DMA на передачу данных в UART3

//#define SetDIRxToRX    GPIO_WriteBit(GPIOA, GPIO_Pin_8,  (BitAction)(0));
//#define SetDIRxToTX    GPIO_WriteBit(GPIOA, GPIO_Pin_8,  (BitAction)(1));

#define U3RXBUFFSIZE  256 //размер буфера приёмника

u8 U3_RX_DATA_READY = 0;//флаг приёма пакета
u8 U3_TX_WAIT = 0;//флаг отправки пакета

const u32 U3BPS[5]={
    4800,//0 
    9600,//1 
    19200,//2 
    57600,//3 
    115200//4     
};

void uart3toUSB_init (void){
  DMA_InitTypeDef DMA_InitStructure;
  USART_InitTypeDef USART_InitStructure;
  //SetDIRxToRX;//драйвер на приём
  uart3data.ID = 3;
  uart3data.DevAddr = FLASH_DATA.Modbus_USB.b[0];//адрес устройства в сети модбас
  uart3data.BPS = FLASH_DATA.Modbus_USB.b[1];//номер элемента массива U3BPS, откуда брать битрейт
  uart3data.Idx = 0;//буфер начать с начала
  uart3data.TXCount = 0;//не известно что передаем
  uart3data.ClntTimeOut = CD_DATA.tdir_Modbus_USB;//задается уставкой, в мкс. время переключения с приема на отправку
  
  //сначала настраиваю ДМА на работу с УАРТом
  //RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);
  DMA_InitStructure.DMA_Channel = DMA_Channel_4;
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &(USART3->DR);
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t) &uart3data.Buffer[0];
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
  DMA_InitStructure.DMA_BufferSize = 0;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
  DMA_Init(DMA1_Stream1, &DMA_InitStructure);//USART3_Rx
  DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
  DMA_Init(DMA1_Stream3, &DMA_InitStructure);//USART3_Tx
  
  //потом настраиваю сам УАРТ и запускаю его
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
  
  USART_InitStructure.USART_BaudRate = U3BPS[uart3data.BPS];
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_Init(USART3, &USART_InitStructure);
 
  //USART3->BRR =  U3BPS[CD_DATA.Modbus_USB.b[1]];//если хотим скорость из уставки
  
  USART3->CR1 |=  USART_CR1_RE;//разрешить приёмник
  USART3->CR1 |=  USART_CR1_TE;//разрешить передатчик 
  USART3->CR1 |=  USART_CR1_UE;//разрешить UART3
  
  U3_TX_WAIT = 0;
  U3_RX_DATA_READY = 0;
  Rx3DMA();//настройка DMA на чтение данных из UART
}

//сравнить BPS и DEVADDR для UART3 если отличаются, то сделать повторнуюю инициализацию

void uart3toUSB_ReInit (void)
{
  USART_InitTypeDef USART_InitStructure;
  if (uart3data.BPS != FLASH_DATA.Modbus_USB.b[1]) {
    uart3data.BPS = FLASH_DATA.Modbus_USB.b[1];
    USART_InitStructure.USART_BaudRate = U3BPS[uart3data.BPS];
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART3, &USART_InitStructure);
  }  
  if (uart3data.ClntTimeOut != CD_DATA.tdir_Modbus_USB) uart3data.ClntTimeOut = CD_DATA.tdir_Modbus_USB;
  if (uart3data.DevAddr != (u16) FLASH_DATA.Modbus_USB.b[0]) uart3data.DevAddr = (u16) FLASH_DATA.Modbus_USB.b[0];  
}


void U3_SetModbusTimerForWaitTransmit(void)
{  
  u16 puls;
  TIM1->CR1 &= (uint16_t)~TIM_CR1_CEN;//остановили таймер
  puls = TIM1->CNT;
  puls += uart3data.ClntTimeOut;
  TIM1->CCR3 = puls;//зарядить канал сравнения на нужную паузу.
  TIM1->SR ^= TIM_IT_CC3; //снять флаг прерывания 
  TIM_ITConfig(TIM1, TIM_IT_CC3, ENABLE);//ждем когда пауза сработает.
  TIM1->CR1 |= TIM_CR1_CEN;//запустили снова
  U3_TX_WAIT = 1;
}

void U3_Timer (void)
{
  TIM_ITConfig(TIM1, TIM_IT_CC3, DISABLE);//запретить прерывания по 2 каналу
  Tx3DMA();//отправить подготовленый пакет
}


u16 U3_SwCNT (void)
{
  if (U3_RX_DATA_READY) 
  {
    U3_RX_DATA_READY = 0;    
    //декодирование команды
    if (command_decode(&uart3data)) 
    {
      if (uart3data.TXCount)
      {
        U3_TX_WAIT = 1;
        //переключить драйвер на отправку
        U3_SetModbusTimerForWaitTransmit();
      }     
      return 1;
    }
    //команда не опознана либо на неё не надо отвечать
    //восстановить приём
      U3_TX_WAIT = 0;
      U3_RX_DATA_READY = 0;
      Rx3DMA();//восстановлением работы приёмника
      return 0;
  }
  else return 0;
}


void Tx3DMA(void) {//настройка DMA на передачу данных в UART
 
  DMA1_Stream3->CR &= ~(uint32_t)DMA_SxCR_EN;//отключаю DMA для получения доступа к регистрам
  USART3->SR  &=  ~USART_SR_TC;   //сбросить флаг окончания передачи
  DMA1->LIFCR = (uint32_t)(DMA_FLAG_FEIF3 | DMA_FLAG_DMEIF3 | DMA_FLAG_TEIF3 | DMA_FLAG_HTIF3 | DMA_FLAG_TCIF3);//почистим флаги стрима ДМА, без этого не работает
  DMA1_Stream3->NDTR = uart3data.TXCount;
  USART3->CR3 |=  USART_CR3_DMAT;
  USART3->CR1 |=  USART_CR1_TE;   //разрешить передатчик
  DMA1_Stream3->CR |= (uint32_t)DMA_SxCR_EN;//включаю DMA
  USART3->CR1 |=  USART_CR1_TCIE; //разрешу прерывания по окончанию передачи
  
} 

void Rx3DMA (void) {//настройка DMA на чтение данных из UART
  
  DMA1_Stream1->CR &= ~(uint32_t)DMA_SxCR_EN;//отключаю DMA для получения доступа к регистрам
  DMA1->LIFCR = (uint32_t)(DMA_FLAG_FEIF1 | DMA_FLAG_DMEIF1 | DMA_FLAG_TEIF1 | DMA_FLAG_HTIF1 | DMA_FLAG_TCIF1);//почистим флаги стрима ДМА, без этого не работает
  USART3->CR3 |=  USART_CR3_DMAR;
  DMA1_Stream1->NDTR =  U3RXBUFFSIZE;
  DMA1_Stream1->CR |= (uint32_t)DMA_SxCR_EN;//включаю DMA
  USART3->CR1 |=  USART_CR1_IDLEIE;//разрешить прерывания по приёму данных
  USART3->CR1 |=  USART_CR1_RE;//разрешить приёмник
}

void USART3_IRQHandler(void)
{  
  SysTick->VAL = 0;
  static u32 IIR;
  IIR = USART3->SR;
    if ((IIR & USART_SR_TC) && (USART3->CR1 & USART_CR1_TCIE)) // Передача окончена (последний байт полностью передан в порт)
      {   
        USART3->SR  &=  ~USART_SR_TC;   //сбросить флаг окончания передачи
        USART3->CR1 &=  ~USART_CR1_TCIE;//запретить прерывание по окончании передачи
        USART3->CR3 &=  ~USART_CR3_DMAT;//запретить UART-ту передавать по DMA
        //USART3->CR1 &=  ~USART_CR1_TE;  //запретить передатчик
        DMA1_Stream3->CR &= ~(uint32_t)DMA_SxCR_EN;//DMA_Cmd(DMA1_Channel7, DISABLE);//выключить DMA передатчика
        //переключить на приём
        Rx3DMA();//настройка DMA на чтение данных из UART
        U3_TX_WAIT = 0;
        return;
      }
    if ((IIR & USART_SR_IDLE) & (USART3->CR1 & USART_CR1_IDLEIE)) // Между байтами при приёме обнаружена пауза в 1 IDLE байт
      {
        USART3->DR; //сброс флага IDLE
        USART3->CR1 &=  ~USART_CR1_RE;    //запретить приёмник
        USART3->CR1 &=  ~USART_CR1_IDLEIE;//запретить прерывания по приёму данных
        USART3->CR3 &=  ~USART_CR3_DMAR;  //запретить DMA RX
        DMA1_Stream1->CR &= ~(uint32_t)DMA_SxCR_EN;//DMA_Cmd(DMA1_Channel6, DISABLE);//выключить DMA на приём
        uart3data.Idx = (u8)(U3RXBUFFSIZE - DMA1_Stream1->NDTR);//кол-во принятых байт
        U3_RX_DATA_READY = 1;//выставляю флаг основному циклу что пакет данных принят
        U3_TX_WAIT = 0;//нет ожидания передачи
      } 
}