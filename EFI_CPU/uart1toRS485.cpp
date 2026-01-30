//modbus rtu интерфейс организующий доступ к параметрам процессора через
//USART to Opt (опц. RS485 интерфейс)
//Используется:
//UART1 ((RX, TX, скорость фиксированная 115200)
//Timer2 (канал сравнения 1) (отсчёт временных интервалов Modbus RTU)

#include "flashdata.h"//тут у нас хранятся уставки для настройки интерфейса
#include "CDdata.h"//тут у нас хранятся уставки для настройки интерфейса
#include "modbus.h"//тут есть функция декодирования принятого сообщения
#include "uart1toRS485.h"


TClient uart1data;
void Rx1DMA (void);//настройка DMA на чтение данных из UART1
//void Tx1DMA (void);//настройка DMA на передачу данных в UART1
void U1_SetModbusTimerForWaitTransmit(void);
#define SetDIR1ToRX    GPIOA->BSRRH |= GPIO_Pin_8
#define SetDIR1ToTX    GPIOA->BSRRL |= GPIO_Pin_8

#define U1RXBUFFSIZE  255 //размер буфера приёмника

u8 U1_RX_DATA_READY = 0;//флаг приёма пакета
u8 U1_TX_WAIT = 0;//флаг отправки пакета

const u32 U1BPS[5]={
    4800,//0 
    9600,//1 
    19200,//2 
    57600,//3 
    115200//4     
};

void uart1to485_init (void){
  DMA_InitTypeDef DMA_InitStructure;
  USART_InitTypeDef USART_InitStructure;
  SetDIR1ToRX;//драйвер на приём
  uart1data.ID = 1;
  uart1data.DevAddr = CD_DATA.MB_RS485_1.b[0];//адрес устройства в сети модбас
  uart1data.BPS = CD_DATA.MB_RS485_1.b[1];//номер элемента массива U2BPS, откуда брать битрейт
  uart1data.Idx = 0;//буфер начать с начала
  uart1data.TXCount = 0;//не известно что передаем
  uart1data.ClntTimeOut = CD_DATA.tdir_Modbus1_RS485;//задается уставкой, в мкс. время переключения с приема на отправку

  
  //сначала настраиваю ДМА на работу с УАРТом  
  DMA_InitStructure.DMA_Channel = DMA_Channel_4;
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &(USART1->DR);
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t) &uart1data.Buffer[0];
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
  DMA_Init(DMA2_Stream2, &DMA_InitStructure);//USART1_Rx
  DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
  DMA_Init(DMA2_Stream7, &DMA_InitStructure);//USART1_Tx
  
  //потом настраиваю сам УАРТ и запускаю его
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
  
  USART_InitStructure.USART_BaudRate = U1BPS[uart1data.BPS];
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_Init(USART1, &USART_InitStructure);
 
    
  USART1->CR1 |=  USART_CR1_RE;//разрешить приёмник
  USART1->CR1 |=  USART_CR1_TE;//разрешить передатчик 
  USART1->CR1 |=  USART_CR1_UE;//разрешить UART2   
  
  U1_TX_WAIT = 0;
  U1_RX_DATA_READY = 0;
  Rx1DMA();//настройка DMA на чтение данных из UART
}

//сравнить BPS и DEVADDR для UART2 если отличаются, то сделать повторнуюю инициализацию
void uart1toModbus_ReInit (void)
{
  USART_InitTypeDef USART_InitStructure;
  if (uart1data.BPS != CD_DATA.MB_RS485_1.b[1]) {
    uart1data.BPS = CD_DATA.MB_RS485_1.b[1];
    USART_InitStructure.USART_BaudRate = U1BPS[uart1data.BPS];
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART2, &USART_InitStructure);
  }  
  if (uart1data.ClntTimeOut != CD_DATA.tdir_Modbus1_RS485) uart1data.ClntTimeOut = CD_DATA.tdir_Modbus1_RS485;
  if (uart1data.DevAddr != (u16)CD_DATA.MB_RS485_1.b[0]) uart1data.DevAddr = (u16)CD_DATA.MB_RS485_1.b[0] ;  
}


void U1_SetModbusTimerForWaitTransmit(void)
{  
  u16 puls;
  TIM1->CR1 &= (uint16_t)~TIM_CR1_CEN;//остановили таймер
  puls = TIM1->CNT;
  puls += uart1data.ClntTimeOut;
  TIM1->CCR1 = puls;//зарядить канал сравнения на нужную паузу.
  TIM1->SR ^= TIM_IT_CC1; //снять флаг прерывания 
  TIM_ITConfig(TIM1, TIM_IT_CC1, ENABLE);//ждем когда пауза сработает.
  TIM1->CR1 |= TIM_CR1_CEN;//запустили снова
  SetDIR1ToTX;
  U1_TX_WAIT = 1;
}

void U1_Timer (void)
{
  TIM_ITConfig(TIM1, TIM_IT_CC1, DISABLE);//запретить прерывания по 2 каналу
  Tx1DMA();//отправить подготовленый пакет
}

u16 U1_SwCNT (void)
{
  if (U1_RX_DATA_READY) {
    U1_RX_DATA_READY = 0;    
    //декодирование команды
    if (command_decode(&uart1data)) 
    {
      if (uart1data.TXCount)
      {
        U1_TX_WAIT = 1;
        //переключить драйвер на отправку
        U1_SetModbusTimerForWaitTransmit();
      }     
      return 1;
    }
    //команда не опознана либо на неё не надо отвечать
    //восстановить приём
      U1_TX_WAIT = 0;
      U1_RX_DATA_READY = 0;
      Rx1DMA();//восстановлением работы приёмника
      return 0;
  }
  else return 0;
}
//--------------------------------------------------




void Tx1DMA(void) {//настройка DMA на передачу данных в UART
 
  DMA2_Stream7->CR &= ~(uint32_t)DMA_SxCR_EN;//отключаю DMA для получения доступа к регистрам
  USART1->SR  &=  ~USART_SR_TC;   //сбросить флаг окончания передачи
  DMA2->HIFCR = (uint32_t)(DMA_FLAG_FEIF7 | DMA_FLAG_DMEIF7 | DMA_FLAG_TEIF7 | DMA_FLAG_HTIF7 | DMA_FLAG_TCIF7);//почистим флаги стрима ДМА, без этого не работает
  DMA2_Stream7->NDTR = uart1data.TXCount;
  USART1->CR3 |=  USART_CR3_DMAT;
  USART1->CR1 |=  USART_CR1_TE;   //разрешить передатчик
  DMA2_Stream7->CR |= (uint32_t)DMA_SxCR_EN;//включаю DMA
  USART1->CR1 |=  USART_CR1_TCIE; //разрешу прерывания по окончанию передачи
  
} 

void Rx1DMA (void) {//настройка DMA на чтение данных из UART
  DMA2_Stream2->CR &= ~(uint32_t)DMA_SxCR_EN;//отключаю DMA для получения доступа к регистрам
  DMA2->LIFCR = (uint32_t)(DMA_FLAG_FEIF2 | DMA_FLAG_DMEIF2 | DMA_FLAG_TEIF2 | DMA_FLAG_HTIF2 | DMA_FLAG_TCIF2);//почистим флаги стрима ДМА, без этого не работает; 
  USART1->CR3 |=  USART_CR3_DMAR;
  DMA2_Stream2->NDTR =  U1RXBUFFSIZE;
  DMA2_Stream2->CR |= (uint32_t)DMA_SxCR_EN;//включаю DMA
  USART1->CR1 |=  USART_CR1_IDLEIE;//разрешить прерывания по приёму данных
  USART1->CR1 |=  USART_CR1_RE;//разрешить приёмник
}

void USART1_IRQHandler(void){
  static u32 IIR;
  IIR = USART1->SR;
    if ((IIR & USART_SR_TC) && (USART1->CR1 & USART_CR1_TCIE)) // Передача окончена (последний байт полностью передан в порт)
      {   
        USART1->SR  &=  ~USART_SR_TC;   //сбросить флаг окончания передачи
        USART1->CR1 &=  ~USART_CR1_TCIE;//запретить прерывание по окончании передачи
        USART1->CR3 &=  ~USART_CR3_DMAT;//запретить UART-ту передавать по DMA
        //USART1->CR1 &=  ~USART_CR1_TE;  //запретить передатчик
        DMA2_Stream7->CR &= ~(uint32_t)DMA_SxCR_EN;//выключить DMA передатчика 
        SetDIR1ToRX;//переключить на приём
        Rx1DMA();//настройка DMA на чтение данных из UART
        U1_TX_WAIT = 0;
        return;
      }
    if ((IIR & USART_SR_IDLE) & (USART1->CR1 & USART_CR1_IDLEIE)) // Между байтами при приёме обнаружена пауза в 1 IDLE байт
      {
        USART1->DR; //сброс флага IDLE
        USART1->CR1 &=  ~USART_CR1_RE;    //запретить приёмник
        USART1->CR1 &=  ~USART_CR1_IDLEIE;//запретить прерывания по приёму данных
        USART1->CR3 &=  ~USART_CR3_DMAR;  //запретить DMA RX
        DMA2_Stream2->CR &= ~(uint32_t)DMA_SxCR_EN;//выключить DMA на приём
        uart1data.Idx = (u8)(U1RXBUFFSIZE - DMA2_Stream2->NDTR);//кол-во принятых байт
        U1_RX_DATA_READY = 1;//выставляю флаг основному циклу что пакет данных принят
        U1_TX_WAIT = 0;//нет ожидания передачи
      }
    
}