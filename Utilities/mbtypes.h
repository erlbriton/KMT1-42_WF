#ifndef __MBTYPES_H
#define __MBTYPES_H
#include "stm32f4xx.h"
		/* FatFs configurations and declarations */

// MODBUS commans structure (common block)
#define _u_dev_addr      0
#define _u_cmd_code      1
#define _u_start_addr_hi 2
#define _u_start_addr_lo 3
#define _u_word_count_hi 4
#define _u_word_count_lo 5
// MODBUS commans structure ( additional data field for 10h command)
#define _u_byte_cnt_cm10     6
#define _u_data_section_cm10 7
// MODBUS commans structure ( additional data field for 03h command)
#define _u_data_section_cm03 3

// MODBUS commans structure (byte count, as rule for transmiting response from device)
#define _u_byte_cnt 2

#define illegal_data_address       0x02
#define error_in_data_size         0x03
#define error_in_request_data_size 0x04
#define error_message_lenght       0x03

typedef
  union TClntStat { // Byte-addressable UINT
     u8 Stat;
	 struct {
           unsigned bReceiveValidPacket : 1; //когда примет валидный пакет, то взведёт этот флаг как сигнал северу
           unsigned bPacketTransmited   : 1; //когда передаст пакет, подаст серверу сигнал, что можно переключаться на других клиентов
           unsigned bWaitForReceive     : 1;
           unsigned bWaitForTransmit    : 1;
	 }bits;
  } TClntStat;

typedef struct _TClient_
{
  u16  ID; //номер интерфейса 0-UART0, 1-UART1, 2-UART2, 3-UART3
  u16  DevAddr;    //Адрес устройства на который нужно реагировать 
  u16  BPS;//скорость связи
  u8   Buffer[256];//тут складируются принятые данные, а так же готовятся пакеты для передачи
  u16  Idx;        //индекс для перемещения по буферу  
  u16  TXCount;    //кол-во байт которое нужно передать  
  u16  ClntTimeOut;//FE для данных поступающих от внешнего интерфейса (скорость вариэйбл)
} TClient;

#endif
