/**
  ******************************************************************************
  * @file    fram.с
  * @author  IMD, Erlina
  * @version V1.0.0
  * @date    18-06-2013
  * @brief   Файл содержит функции для работы с с ключом параметров на Fram 
  * 
  ******************************************************************************
  * @Как с этим работать:
  * - Сначала пины портов микроконтроллера, которые являются управляющими 
  * сигналами FRAM. Далее в init.c конфигурируются порты микроконтроллера. 
  * - В файле fram.h редактируются макросы для сигналов управления для данной 
  * аппаратной реализации (если необходимо).
  * - В функии инициализации устройства вызывается функция check_fram(), которая
  * проверяет работу ключа параметров и целостность данных, на нем записаных.
  * - Если вызвана функция записи во собственную flash память устройства, 
  * вызвать функцию fram_wr_massive для записи в основной и резервный сектора 
  * ключа параметров измененные данные из flash памяти (чтобы на ключе 
  * параметров всегда были актуальные данные)
  * - Так же нижеописанные функции можно вызывать по необходимости (например, 
  * если flash-память повреждена - записать в неё уставки из ключа параметром, 
  * предварительно считав её с помощью fram_rd_massive)
  ******************************************************************************
  * @Заметки:
  *
  * 
  * 
  * 
  * 
  * 
  *
  * 
  ******************************************************************************
  */
#include "fram.h"
#include "crc16.h"//модуль с функциями для расчета контрольной суммы массива

// команды для работы с FRAM
#define FREAD 0x03      //Read Status Register 0 - 255
#define FWRITE 0x02     //Write Status Register
#define FREAD_bkp 0x0B  //Read Status Register 256- 511
#define FWRITE_bkp 0x0A  //Write Status Register

#define WREN 0x06       //Set Write Enable Latch
#define WRDI 0x04       //Write Disable 	
#define RDSR 0x05       //Read Status Register
#define WRSR 0x01       //Write Status Register

/* Ниже представлены  определения и макросы для сигналов управления FRAM
  ******************************************************************************
  * Fram_SIO - линия данных 
  * Fram_SCK - линия тактирования 
  * Fram_CS - Chip select  
  * 
  * Fram_SIO_IN - состояние линии данных
  * MACROS_on - выставить сигнал в 1
  * MACROS_off - выставить сигнал в 0
  * 
  * Макросы прописываются индивидуально для каждого приложения 
  * (зависит от того, какой микроконтроллер используется, 
  * на какие пины каких портов посажены сигналы управления, смотри схему)
  ******************************************************************************
  */
#define Fram_SIO GPIO_Pin_3 
#define Fram_SCK GPIO_Pin_5 
#define Fram_CS  GPIO_Pin_4 

#define Fram_SIO_IN GPIOB->IDR & Fram_SIO
#define Fram_SIO_on GPIOB->BSRRL = GPIO_Pin_3
#define Fram_SCK_on GPIOB->BSRRL = GPIO_Pin_5
#define Fram_CS_on  GPIOB->BSRRL = GPIO_Pin_4  

#define Fram_SIO_off GPIOB->BSRRH = GPIO_Pin_3 
#define Fram_SCK_off GPIOB->BSRRH = GPIO_Pin_5 
#define Fram_CS_off  GPIOB->BSRRH = GPIO_Pin_4  

//другие определения
#define delay 30 //задержка 250нс =5
#define delay_sck 30 //задержка для клока

u8 ReadSR(void);
void fram_pulse(u8 Delay);//тактирование одного бита
void fram_wr_byte(u8 Data_byt);//передача одного байта, данные в функцию отправляются сразу
void fram_wr_byte(u8 Data_byt); //чтение одного байта, возвращается байт
void Fram_CS_Up(void);
void Fram_CS_Down(void);
void Fram_sio_mode(u8 mode);
//буфер для фрам
u8 Fram_Buffer[FramTmpBufferSize];

/*функции записи чтения данных во фрам FM25040*/
/**
  * @brief  Записать байт данных во fram
  * @param  Data_byt - данные для передачи
  * @retval none.
  */
void fram_wr_byte(u8 Data_byt)
{
  //7бит
  if((Data_byt & 0x80) == 0) {Fram_SIO_off;}
  else {Fram_SIO_on;}
  fram_pulse(delay_sck);
  //6бит
  if((Data_byt & 0x40) == 0) {Fram_SIO_off;}
  else {Fram_SIO_on;}
  fram_pulse(delay_sck);
  //5бит
  if((Data_byt & 0x20) == 0) {Fram_SIO_off;}
  else {Fram_SIO_on;}
  fram_pulse(delay_sck);
  //4бит
  if((Data_byt & 0x10) == 0) {Fram_SIO_off;}
  else {Fram_SIO_on;}
  fram_pulse(delay_sck);
  //3бит
  if((Data_byt & 0x08) == 0) {Fram_SIO_off;}
  else {Fram_SIO_on;}
  fram_pulse(delay_sck);
  //2бит
  if((Data_byt & 0x04) == 0) {Fram_SIO_off;}
  else {Fram_SIO_on;}
  fram_pulse(delay_sck);
  //1бит
  if((Data_byt & 0x02) == 0) {Fram_SIO_off;}
  else {Fram_SIO_on;}
  fram_pulse(delay_sck);
  //0бит
  if((Data_byt & 0x01) == 0) {Fram_SIO_off;}
  else {Fram_SIO_on;}
  fram_pulse(delay_sck);

}
/**
  * @brief  формирование импульса тактирования fram SCK 1->0
  * @param  Delay - задержка для установления уровня
  * @retval none.
  */
void fram_pulse(u8 Delay)
{
  u8 i;
  for(i=Delay;i;i--){}
  Fram_SCK_on;//on
  for(i=(Delay+Delay);i;i--){}
  Fram_SCK_off;//off
  for(i=Delay;i;i--){} 
}

/**
  * @brief  прочитать один байт
  * @param  none 
  * @retval прочитанный байт данных
  */
u8 fram_rd_byte(void)
{
  u8 Data = 0;
  //7бит
  if (Fram_SIO_IN)  Data = Data | 0x80; 
  else Data = 0;
  fram_pulse(delay_sck);
  //6бит
   if (Fram_SIO_IN)  Data = Data | 0x40;  
  fram_pulse(delay_sck);
  //5бит
  if (Fram_SIO_IN)  Data = Data | 0x20; 
  fram_pulse(delay_sck);
  //4бит
  if (Fram_SIO_IN)  Data = Data | 0x10; 
  fram_pulse(delay_sck);
  //3бит
  if (Fram_SIO_IN)  Data = Data | 0x08; 
  fram_pulse(delay_sck);
  //2бит
  if (Fram_SIO_IN)  Data = Data | 0x04; 
  fram_pulse(delay_sck);
  //1бит
  if (Fram_SIO_IN)  Data = Data | 0x02; 
  fram_pulse(delay_sck);
  //0бит
  if (Fram_SIO_IN) Data = Data | 0x01;  
  fram_pulse(delay_sck);
  return Data;
}

/**
  * @brief  записать весь массив даных
  * @param  *massive - указатель на массив, откуда берем данные для записи
            byte_num - количество записываемых байт (от 1 до 256)
            Addr - адрес, с которого начинать брать данные/куда записывать (от 0 до 255)
            Mem_half - указывает, куда пишем 0 - основной сектор, 1 - backup 
  * @retval none
  */
void fram_wr_massive(u8 *massive, u16 byte_num, u8 Addr, u8 Mem_half) 
{
  u16 i =0;
  //cs вверх
  Fram_CS_Up();
  Fram_CS_Down();
  fram_wr_byte(WRSR);
  fram_wr_byte(0x00);
  Fram_CS_Up();
  //fram_wren: разрешение записи
  Fram_CS_Down();
    fram_wr_byte(WREN);
    Fram_CS_Up();
    Fram_CS_Down();
  if (Mem_half) fram_wr_byte(FWRITE_bkp);
  else fram_wr_byte(FWRITE);
  fram_wr_byte(Addr);
  //а теперь собственно пишем массив, n число байт
  for(i= Addr ;i< byte_num ;i++)
  {
    fram_wr_byte(massive[i]);
  }
  Fram_CS_Up();
}

/**
  * @brief  чтение массива данных
  * @param  *massive - указатель на массив, откуда берем данные для записи
            byte_num - количество записываемых байт (от 1 до 256)
            Addr - адрес, с которого начинать брать данные/куда записывать (от 0 до 255)
            Mem_half - указывает, откуда пишем 0 - основной сектор, 1 - backup 
  * @retval none
  */                                                                    
void fram_rd_massive(u8 *massive, u16 n, u8 Addr, u8 Mem_half)
{
  u16 i =0;
  Fram_CS_Down();
  for(i=delay;i;i--){}
  if (Mem_half) fram_wr_byte(FREAD_bkp);
  else fram_wr_byte(FREAD);
  fram_wr_byte(Addr);
  for(i=delay;i;i--){}
  for(i=delay;i;i--){}
  Fram_sio_mode(1);//ногу порта на вход настроили
  for(i=delay;i;i--){}
  //читаем массив данных
  for(i=Addr;i<n;i++)
  {
    massive[i] = fram_rd_byte();
  }
  for(i=delay;i;i--){}
  Fram_CS_Up();
  Fram_sio_mode(0);//ногу порта на вsход настроили
}  

/**
  * @brief  прочитать данные статус-регистра fram (есть запрет записи или нет)
  * @param  none 
  * @retval прочитанный байт данных
  */
u8 ReadSR(void)   
{   
  u8 cData = 0;   
  Fram_CS_off;   
  fram_wr_byte(RDSR);
  Fram_sio_mode(1);//ногу порта на вход настроили
  cData=fram_rd_byte();   
  Fram_CS_on;   
  Fram_sio_mode(0);//ногу порта на вход настроили
  return cData;   
}

/**
  * @brief  чип-селект установить в 1
  * @param  none 
  * @retval none
  */
void Fram_CS_Up(void)
{
  u8 i =0;
  for(i=delay;i;i--){}
  Fram_CS_on;
  Fram_SCK_on;//on
  Fram_SIO_on;
  for(i=delay;i;i--){}
}

/**
  * @brief  сбросить чип-селект
  * @param  none 
  * @retval none
  */
void Fram_CS_Down(void)
{
  u8 i =0;
  for(i=delay;i;i--){}
  for(i=delay;i;i--){}
  Fram_SIO_on;
  Fram_SCK_off;
  for(i=delay;i;i--){}
  Fram_CS_off;
  for(i=delay;i;i--){}
}

/**
  * @brief  настройка пина на вход/выход, в зависимости от чтения/записи
  * @param  mode - параметр настройки пина //0 - на выход, 1 - на вход
  * @retval none
  */
void Fram_sio_mode(u8 mode) 
{
  GPIO_InitTypeDef GPIO_InitStructure;

  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//по умолчанию скорость 50МГц
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;//по умолчанию без притяжки
  if(mode == 0) 
  {
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//мод - out выходы пишем данные
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD; //output-drain
  }
  else 
  {
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN; //вход, читаем данные
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//по умолчанию выход пушпульный
  }
  //ноги на фрам память,            FRAM_sio менять на ходу
  GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_3;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
}

/**
  * @brief  проверка фрам
  * @param none
  * @retval CRC_16
  */
u8 check_fram (void)
{
  
  u8 OutVal=0;
  
  fram_rd_massive(Fram_Buffer, FramTmpBufferSize, 0x00, fram_sector);//читаем основной сектор фрам
  if (crc16((u8*)&Fram_Buffer, FramTmpBufferSize))
  {    
    //контрольная сумма не сошлась, сектор битый или после прошивки
    fram_rd_massive(Fram_Buffer, FramTmpBufferSize, 0x00, fram_bkp);//читаем резервный сектор фрам
    if (crc16((u8*)&Fram_Buffer, FramTmpBufferSize)) 
    {
      OutVal = FRAM_BKP_ERROR + FRAM_ERROR; //оба сектора битые или после прошивки
    }
    else  
    {
      fram_wr_massive(Fram_Buffer, FramTmpBufferSize, 0x00, fram_sector);//пытаемся восстановить из бэкапа
      if (crc16((u8*)&Fram_Buffer, FramTmpBufferSize))   OutVal = FRAM_BKP_ERROR; //не получилось, ставим ошибку
    }    
  }
  
  return OutVal;
}
