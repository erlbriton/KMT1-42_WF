#include "mbtypes.h"
#include "bastypes.h"
#include "modbus.h"
#include "crc16.h"
#include "ramdata.h"
#include "memutil.h"
#include "flashdata.h"
#include "CDdata.h"
#include "id.h"
//#include "uart3toRS485.h"



void ModbusRamRead(u32 DATA_BASE, TClient *pC);
void ModbusRamWrite(TClient *pC);//u32 DATA_BASE, 
void ModbusCDWrite(TClient *pC);//запись калибровочных данных u32 DATA_BASE, 
void ModbusFlashWrite(TClient *pC);//u32 DATA_BASE, 

void frame_end(TClient *pC)
{
  bauint crc;
  crc.i = crc16(&pC->Buffer[0], pC->TXCount);//подсчет CRC буфера
  //запись CRC в конец буфера
  pC->Buffer[pC->TXCount++] = crc.b[1];//crc >> 8;//crc;//crc >> 8     ;//CRCHi;
  pC->Buffer[pC->TXCount++] = crc.b[0];//crc;//crc >> 8;//crc  ;//CRCLo;
}

u8 GetDeviceIDLength(void)
{
u8 i=0;
 while (DeviceID[i++]!=0);
 return i;
}

void GetDeviceID(TClient *pC)
{
u8 i=0;
u8 j;
   j = pC->Buffer[_u_byte_cnt] = pC->TXCount = GetDeviceIDLength();
   do
     pC->Buffer[_u_data_section_cm03+i]=DeviceID[i];
   while ((i++)!=j);
   pC->TXCount+=2;//прибавить длину заголовка
   frame_end(pC);
}

bool ModbusMemRead(TClient *pC)
{
  u16 addrtt;
  //static u8 i0,i1,i2,i3;
  // по номерам регистров проверить какую область памяти пытается прочесть юзер,
  

  //получаю номер регистра в w.i
 
  addrtt = (pC->Buffer[_u_start_addr_hi])<<8;//
  addrtt += pC->Buffer[_u_start_addr_lo];//
  //выбор программы чтения памяти
  //RAM_DATA
    
  //оперативные данные 
    if ((addrtt >= r_min_RAM_DATA)&&(addrtt < r_max_RAM_DATA))
    {
      ModbusRamRead((u32)&RAM_DATA, pC);
      return (TRUE);
    }  
  
  
  //FLASH_DATA данные уставок 
  if ((addrtt >= r_min_FLASH_DATA)&&(addrtt < r_max_FLASH_DATA)) 
  {
    ModbusRamRead((u32)&FLASH_DATA, pC);
    return (TRUE);
  }
  
  //CD_DATA данные калибровочные 
  if ((addrtt >= r_min_CD_DATA)&&(addrtt < r_max_CD_DATA))
  {
    ModbusRamRead((u32)&CD_DATA, pC);
    return (TRUE);
  }

  //не нашёл подходящей области
  //ModbusDataRetranslate(&lnks_RTR_WRITE, pC, TRUE);//пробую ретранслировать, вдруг Устройство знает
  return (FALSE);
}




bool ModbusMemWrite(TClient *pC){
  
 u16 addrtt;
 addrtt = (pC->Buffer[_u_start_addr_hi])<<8;//
  addrtt += pC->Buffer[_u_start_addr_lo];//
  
  //выбор программы записи памяти
  
  
  //RAM_DATA
  if ((addrtt >= r_min_RAM_DATA) && (addrtt < r_max_RAM_DATA))
  {
    ModbusRamWrite(pC);//(u32)&RAM_DATA, 
    return(TRUE);
  } 

  //Уставки 
  
  if ((addrtt >= r_min_FLASH_DATA) && (addrtt <= r_max_FLASH_DATA)) 
  {  
    ModbusFlashWrite(pC);//(u32)&FLASH_DATA, 
    //сравнить BPS и DEVADDR для UART2 если отличаются, то сделать повторнуюю инициализацию    
    //uart2rs485_ReInit();?????????????
    return (TRUE);
  }
  
  //калибровочные данные
  if ((addrtt >= r_min_CD_DATA) && (addrtt <= r_max_CD_DATA)) 
  {  
    ModbusCDWrite(pC);//(u32)&CD_DATA, 
    uart2to485_ReInit();

    return (TRUE);
  }
  //не нашёл подходящей области
  //ModbusDataRetranslate(&lnks_RTR_WRITE, pC, TRUE);//пробую ретранслировать, вдруг Устройство знает
  return (FALSE);
}

void ModbusRamRead(u32 DATA_BASE, TClient *pC)
{
  bauint w;
  u32 i;
  //RAM_DATA_BASE начало адресов ОЗУ, для получения абсолютного адреса
  //к значению полученному из номера регистра прибавить RAM_DATA_BASE
  //1) получить из буфера номер регистра
  //1.1) преобразовать его в адрес в памяти
  w.b[0] = pC->Buffer[_u_start_addr_lo];
  w.b[1] = pC->Buffer[_u_start_addr_hi] & 0x0F;
  i = w.i;
  i = (i << 1) & 0x1FFF;
  u16 *ModbusAddrSet = (u16 *)(DATA_BASE + i);
  //2)получим кол-во передаваемых регистров
  u8 ModbusAddrCount = pC->Buffer[_u_word_count_lo];
  //4) заполнение буфера
  //4.1) записать в буфер кол-во передаваемых байт
  pC->Buffer[_u_byte_cnt]=pC->TXCount=(u16)(ModbusAddrCount << 1);
  pC->TXCount+=3;
  //4.2) записать в буфер содержимое регистров (старшими байтами вперёд)
  u8 j = _u_data_section_cm03;
  do {
    w.i = *ModbusAddrSet++;
    pC->Buffer[j++]= w.b[1];
    pC->Buffer[j++]= w.b[0];
  } while (--ModbusAddrCount != 0);
  frame_end(pC);
}

void ModbusRamWrite(TClient *pC){ //u32 DATA_BASE
  bauint w;
  u32 i;
  //1) получить из буфера номер регистра
  //1.1) преобразовать его в адрес в памяти
  w.b[0] = pC->Buffer[_u_start_addr_lo];
  w.b[1] = pC->Buffer[_u_start_addr_hi] & 0x0F;
  i = w.i;
  i = (i << 1) & 0x1FFF;
  u16 *ModbusAddrSet = (u16 *)(((u32)&RAM_DATA) + i);//DATA_BASE
  //2)получим кол-во переданных регистров
  u8 ModbusAddrCount = pC->Buffer[_u_word_count_lo];
  //3)чтение буфера в память
  u8 j = _u_data_section_cm10;
  do {
    w.b[1] = pC->Buffer[j++];
    w.b[0] = pC->Buffer[j++];
    *ModbusAddrSet++ = w.i;
  } while (--ModbusAddrCount != 0);
  pC->TXCount = 6;
  frame_end(pC);
}

void ModbusCDWrite(TClient *pC)//u32 DATA_BASE, 
{
  bauint w;
  u32 i;
  //bauint crc, _crc;
  
  CopyFlashToTmpBuffer((u32)&CD_DATA);//DATA_BASE
  //1) получить из буфера номер регистра
  //1.1) преобразовать его в адрес в памяти
  w.b[0] = pC->Buffer[_u_start_addr_lo];
  w.b[1] = pC->Buffer[_u_start_addr_hi] & 0x0F;
  i = w.i;
  i = (u32)&FlashTmpBuffer + (u32)(i << 1);//адрес во временном буфере
  u16 *ModbusAddrSet = (u16*)i;
  //2)получим кол-во переданных регистров
  u8 ModbusAddrCount = pC->Buffer[_u_word_count_lo];
  //3)внести изменения во временный буфер со свопом байт
  u8 j = _u_data_section_cm10;
  do {
    w.b[1] = pC->Buffer[j++];
    w.b[0] = pC->Buffer[j++];
    *ModbusAddrSet++ = w.i;
  } while (--ModbusAddrCount != 0);
  //4)данные находятся во временном буфере, теперь:
  //записать данные из временного буфера
  __disable_irq(); // handles nested interrupt
  FlashSectorWrite((u32)&CD_DATA, (u32) &FlashTmpBuffer);  
  __enable_irq(); // handles nested interrupt
  pC->TXCount = 6;
  frame_end(pC);
}

void ModbusFlashWrite(TClient *pC){//u32 DATA_BASE,
  u8 flash_ok=0;
  bauint w;
  u32 i;
  volatile bauint crc, _crc;
  //FLASH_DATA_BASE начало основного сектора, для получения абсолютного адреса
  //к значению полученному из номера регистра прибавить FLASH_DATA_BASE
  //0)копировать основной сектор флэша во временный буфер
  CopyFlashToTmpBuffer((u32)&FLASH_DATA);//DATA_BASE
  //1) получить из буфера номер регистра
  //1.1) преобразовать его в адрес в памяти
  w.b[0] = pC->Buffer[_u_start_addr_lo];
  w.b[1] = pC->Buffer[_u_start_addr_hi] & 0x0F;
  i = w.i;
  i = (u32)&FlashTmpBuffer + (u32)(i << 1);//адрес во временном буфере
  u16 *ModbusAddrSet = (u16*)i;
  //2)получим кол-во переданных регистров
  u8 ModbusAddrCount = pC->Buffer[_u_word_count_lo];
  //3)внести изменения во временный буфер со свопом байт
  u8 j = _u_data_section_cm10;
  do {
    w.b[1] = pC->Buffer[j++];
    w.b[0] = pC->Buffer[j++];
    *ModbusAddrSet++ = w.i;
  } while (--ModbusAddrCount != 0);
  //4)данные находятся во временном буфере, теперь:
  //4.1)Подсчитать контрольную сумму временного буфера
    crc.i = crc16((u8*)&FlashTmpBuffer, FlashTmpBufferSize-2);//подсчет CRC буфера
  //4.2)добавить контрольную сумму в конец временного буфера
    _crc.b[0] = crc.b[1];
    _crc.b[1] = crc.b[0];
    FlashTmpBuffer.wFlashTmpBuffer[FlashTmpBufferSize_w-1] = _crc.i;
    //5)стереть резервный сектор флэша
  //6)записать туда данные из временного буфера
    
   __disable_irq(); // handles nested interrupt
   FlashSectorWrite((u32)&BKFLASH_DATA, (u32) &FlashTmpBuffer);//BK
  //6.1)Проверить CRC буфера, если не испортился писать дальше
  if (crc16((u8*)&BKFLASH_DATA, FlashTmpBufferSize)==0)
  {
       //7)стереть основной сектор флэша
       //8)записать туда данные из временного буфера
       FlashSectorWrite((u32)&FLASH_DATA, (u32) &FlashTmpBuffer);
       flash_ok=1;
  }
  else 
  {
        //Блок оказался кривым, резервный сектор флеша испорчен.
        //Попытаться Восстановить из основного, если не получится
        //не пытаться переписать основной сектор
        FlashSectorWrite((u32)&BKFLASH_DATA, (u32)&FLASH_DATA);
        //тут по идее какую нибудь ошибку бы выдать
  };
  __enable_irq(); // handles nested interrupt

  pC->TXCount = 6;
  frame_end(pC);
}

bool command_decode(TClient *pC)
{
  u8 cmd;
  //проверка контрольной суммы пакета
  if (crc16(&pC->Buffer[0], pC->Idx) == 0)
  {
    //проверка адреса устройствa
    if (pC->Buffer[_u_dev_addr] == pC->DevAddr)
    {
      //адрес совпал, контрольная сумма пакета правильная
      //Расшифровка поля комад
      cmd = pC->Buffer[_u_cmd_code];
      switch (cmd){
	case 0x03: if (ModbusMemRead(pC)) 
        {
          return (TRUE);//чтение
        }
                   return (FALSE);
	case 0x10: if (ModbusMemWrite(pC))
        {
          return (TRUE);
        }//запись
                   return (FALSE);
	case 0x11: GetDeviceID(pC);//чтение ID-строки
                   return (TRUE); //
	default:  return (FALSE);//команда не распознана
      };
    };
  };
  return (FALSE);//контрольная сумма не сошлась, либо адрес не совпал
}
