//modbus rtu интерфейс организующий доступ к параметрам процессора через
//USART to RS485 интерфейс
//Используется:
//UART2 (RX, TX, скорость изменяемая)
//Timer2 (канал сравнения 2) (отсчёт временных интервалов Modbus RTU)

#include "flashdata.h"//тут у нас хранятся уставки для настройки интерфейса
#include "CDdata.h"//тут у нас хранятся уставки для настройки интерфейса
#include "modbus.h"//тут есть функция декодирования принятого сообщения
#include "uart2toRS485.h"

TClient uart2data;
void Rx2DMA (void);//настройка DMA на чтение данных из UART2
void Tx2DMA (void);//настройка DMA на передачу данных в UART2
TFLASH_DATA FLASH_DATA;

//#define SetDIR2ToRX    GPIOC->BSRRH |= GPIO_Pin_12
#define SetDIR2ToTX    GPIOC->BSRRL |= GPIO_Pin_12

#define U2RXBUFFSIZE  255 //размер буфера приёмника

u8 U2_RX_DATA_READY = 0;//флаг приёма пакета
u8 U2_TX_WAIT = 0;//флаг отправки пакета

const u32 U2BPS[5]={
    4800,//0 
    9600,//1 
    19200,//2 
    57600,//3 
    115200//4     
};

void uart2to485_init (void){
  DMA_InitTypeDef DMA_InitStructure;
  USART_InitTypeDef USART_InitStructure;
 // SetDIR2ToRX;//драйвер на приём
  uart2data.ID = 2;
  uart2data.DevAddr = FLASH_DATA.MB_RS485_2.b[0];//адрес устройства в сети модбас
  uart2data.BPS = FLASH_DATA.MB_RS485_2.b[1];//номер элемента массива U2BPS, откуда брать битрейт
  uart2data.Idx = 0;//буфер начать с начала
  uart2data.TXCount = 0;//не известно что передаем
  uart2data.ClntTimeOut = CD_DATA.tdir_Modbus2_RS485;//задается уставкой, в мкс. время переключения с приема на отправку
  
  
  //сначала настраиваю ДМА на работу с УАРТом
  //RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);
  DMA_InitStructure.DMA_Channel = DMA_Channel_4;
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &(USART2->DR);
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t) &uart2data.Buffer[0];
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
  DMA_Init(DMA1_Stream5, &DMA_InitStructure);//USART2_Rx
  DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
  DMA_Init(DMA1_Stream6, &DMA_InitStructure);//USART2_Tx
  
  //потом настраиваю сам УАРТ и запускаю его
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
  
  USART_InitStructure.USART_BaudRate = U2BPS[uart2data.BPS];
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_Init(USART2, &USART_InitStructure);
 
  //USART2->BRR =  U2BPS[CD_DATA.Modbus_USB.b[1]];//если хотим скорость из уставки
  
  USART2->CR1 |=  USART_CR1_RE;//разрешить приёмник
  USART2->CR1 |=  USART_CR1_TE;//разрешить передатчик 
  USART2->CR1 |=  USART_CR1_UE;//разрешить UART2  
  
  U2_TX_WAIT = 0;
  U2_RX_DATA_READY = 0;
  Rx2DMA();//настройка DMA на чтение данных из UART
}

//сравнить BPS и DEVADDR для UART2 если отличаются, то сделать повторнуюю инициализацию

void uart2to485_ReInit (void)
{
  USART_InitTypeDef USART_InitStructure;
  if (uart2data.BPS != FLASH_DATA.MB_RS485_2.b[1]) {
    uart2data.BPS = FLASH_DATA.MB_RS485_2.b[1];
    USART_InitStructure.USART_BaudRate = U2BPS[uart2data.BPS];
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART2, &USART_InitStructure);
  }  
  if (uart2data.ClntTimeOut != CD_DATA.tdir_Modbus2_RS485) uart2data.ClntTimeOut = CD_DATA.tdir_Modbus2_RS485;
  if (uart2data.DevAddr != (u16) FLASH_DATA.MB_RS485_2.b[0]) uart2data.DevAddr = (u16) FLASH_DATA.MB_RS485_2.b[0];  
}


void U2_SetModbusTimerForWaitTransmit(void)
{  
  u16 puls;
  TIM1->CR1 &= (uint16_t)~TIM_CR1_CEN;//остановили таймер
  puls = TIM1->CNT;
  puls += uart2data.ClntTimeOut;
  TIM1->CCR2 = puls;//зарядить канал сравнения на нужную паузу.
  TIM1->SR ^= TIM_IT_CC2; //снять флаг прерывания 
  TIM_ITConfig(TIM1, TIM_IT_CC2, ENABLE);//ждем когда пауза сработает.
  TIM1->CR1 |= TIM_CR1_CEN;//запустили снова
  SetDIR2ToTX;
  U2_TX_WAIT = 1;
}

void U2_Timer (void)
{
  TIM_ITConfig(TIM1, TIM_IT_CC2, DISABLE);//запретить прерывания по 2 каналу
  Tx2DMA();//отправить подготовленый пакет
}


u16 U2_SwCNT (void){
  if (U2_RX_DATA_READY) {
    U2_RX_DATA_READY = 0;    
    //декодирование команды
    if (command_decode(&uart2data)) 
    {
      if (uart2data.TXCount)
      {
        U2_TX_WAIT = 1;
        //переключить драйвер на отправку
        U2_SetModbusTimerForWaitTransmit();
      }     
      return 1;
    }
    //команда не опознана либо на неё не надо отвечать
    //восстановить приём
      U2_TX_WAIT = 0;
      U2_RX_DATA_READY = 0;
      Rx2DMA();//восстановлением работы приёмника
      return 0;
  }
  else return 0;
}


void Tx2DMA(void) {//настройка DMA на передачу данных в UART
 
  DMA1_Stream6->CR &= ~(uint32_t)DMA_SxCR_EN;//отключаю DMA для получения доступа к регистрам
  USART2->SR  &=  ~USART_SR_TC;   //сбросить флаг окончания передачи
  DMA1->HIFCR = (uint32_t) (DMA_FLAG_FEIF6 | DMA_FLAG_DMEIF6 | DMA_FLAG_TEIF6 | DMA_FLAG_HTIF6 | DMA_FLAG_TCIF6);//почистим флаги стрима ДМА, без этого не работает  
  DMA1_Stream6->NDTR = uart2data.TXCount;
  USART2->CR3 |=  USART_CR3_DMAT;
  USART2->CR1 |=  USART_CR1_TE;   //разрешить передатчик
  DMA1_Stream6->CR |= (uint32_t)DMA_SxCR_EN;//включаю DMA
  USART2->CR1 |=  USART_CR1_TCIE; //разрешу прерывания по окончанию передачи
  
} 

void Rx2DMA (void) {//настройка DMA на чтение данных из UART
  
  DMA1_Stream5->CR &= ~(uint32_t)DMA_SxCR_EN;//отключаю DMA для получения доступа к регистрам
  DMA1->HIFCR = (uint32_t) (DMA_FLAG_FEIF5 | DMA_FLAG_DMEIF5 | DMA_FLAG_TEIF5 | DMA_FLAG_HTIF5 | DMA_FLAG_TCIF5);//почистим флаги стрима ДМА, без этого не работает 
  USART2->CR3 |=  USART_CR3_DMAR;
  DMA1_Stream5->NDTR =  U2RXBUFFSIZE;
  DMA1_Stream5->CR |= (uint32_t)DMA_SxCR_EN;//включаю DMA
  USART2->CR1 |=  USART_CR1_IDLEIE;//разрешить прерывания по приёму данных
  USART2->CR1 |=  USART_CR1_RE;//разрешить приёмник
}

void USART2_IRQHandler(void)
{  
  static u32 IIR;
  IIR = USART2->SR;
    if ((IIR & USART_SR_TC) && (USART2->CR1 & USART_CR1_TCIE)) // Передача окончена (последний байт полностью передан в порт)
      {   
        USART2->SR  &=  ~USART_SR_TC;   //сбросить флаг окончания передачи
        USART2->CR1 &=  ~USART_CR1_TCIE;//запретить прерывание по окончании передачи
        USART2->CR3 &=  ~USART_CR3_DMAT;//запретить UART-ту передавать по DMA
        //USART2->CR1 &=  ~USART_CR1_TE;  //запретить передатчик
        DMA1_Stream6->CR &= ~(uint32_t)DMA_SxCR_EN;//DMA_Cmd(DMA1_Channel7, DISABLE);//выключить DMA передатчика
        //SetDIR2ToRX;//переключить на приём
        Rx2DMA();//настройка DMA на чтение данных из UART
        U2_TX_WAIT = 0;
        return;
      }
    if ((IIR & USART_SR_IDLE) & (USART2->CR1 & USART_CR1_IDLEIE)) // Между байтами при приёме обнаружена пауза в 1 IDLE байт
      {
        USART2->DR; //сброс флага IDLE
        USART2->CR1 &=  ~USART_CR1_RE;    //запретить приёмник
        USART2->CR1 &=  ~USART_CR1_IDLEIE;//запретить прерывания по приёму данных
        USART2->CR3 &=  ~USART_CR3_DMAR;  //запретить DMA RX
        DMA1_Stream5->CR &= ~(uint32_t)DMA_SxCR_EN;//DMA_Cmd(DMA1_Channel6, DISABLE);//выключить DMA на приём
        uart2data.Idx = (u8)(U2RXBUFFSIZE - DMA1_Stream5->NDTR);//кол-во принятых байт
        U2_RX_DATA_READY = 1;//выставляю флаг основному циклу что пакет данных принят
        U2_TX_WAIT = 0;//нет ожидания передачи
      }
}